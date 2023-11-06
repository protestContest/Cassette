#include "project.h"
#include "parse.h"
#include "runtime/env.h"
#include "module.h"
#include "source.h"
#include "univ/string.h"
#include "univ/system.h"
#include "runtime/ops.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  Val entry;
  HashMap modules;
  Mem mem;
  SymbolTable symbols;
  Manifest *manifest;
} Project;

static void InitProject(Project *p, Manifest *manifest);
static void DestroyProject(Project *p);
static Result ParseModules(Project *project);
static Result ScanDependencies(Project *project);
static Result CompileProject(Val build_list, Chunk *chunk, Project *p);

void InitManifest(Manifest *manifest)
{
  manifest->name = 0;
  InitVec((Vec*)&manifest->files, sizeof(char*), 8);
}

void DestroyManifest(Manifest *manifest)
{
  u32 i;
  for (i = 0; i < manifest->files.count; i++) {
    free(manifest->files.items[i]);
  }
  DestroyVec((Vec*)&manifest->files);
}

bool ReadManifest(char *filename, Manifest *manifest)
{
  char *source;
  char *cur;
  char *basename = Basename(filename, '/');

  source = ReadFile(filename);
  if (source == 0) return false;

  cur = SkipBlankLines(source);
  while (*cur != 0) {
    char *end = LineEnd(cur);
    if (*end == '\n') {
      /* replace newline with string terminator */
      *end = 0;
      end++;
    }
    /* add full path filename */
    ObjVecPush(&manifest->files, JoinStr(basename, cur, '/'));
    cur = end;

    /* skip to next non-space character */
    cur = SkipBlankLines(cur);
  }

  free(source);
  free(basename);

  return true;
}

Result BuildProject(Manifest *manifest, Chunk *chunk)
{
  Result result;
  Project project;

  InitProject(&project, manifest);

  result = ParseModules(&project);
  result = ScanDependencies(&project);
  result = CompileProject(result.value, chunk, &project);

  DestroyProject(&project);
  return OkResult(Nil);
}

static void InitProject(Project *p, Manifest *manifest)
{
  p->entry = Nil;
  p->manifest = manifest;
  InitHashMap(&p->modules);
  InitMem(&p->mem, 1024);
  InitSymbolTable(&p->symbols);
}

static void DestroyProject(Project *p)
{
  DestroyHashMap(&p->modules);
  DestroyMem(&p->mem);
  DestroySymbolTable(&p->symbols);
  p->entry = Nil;
  p->manifest = 0;
}

/* parses each file and puts the result in a hashmap, keyed by module name (or
   filename if it isn't a module) */
static Result ParseModules(Project *project)
{
  Result result;
  Parser p;
  u32 i;

  InitParser(&p, &project->mem, &project->symbols);

  for (i = 0; i < project->manifest->files.count; i++) {
    Val name;
    result = ParseModule(project->manifest->files.items[i], &p);
    if (!result.ok) return result;

    name = ModuleName(result.value, &project->mem);

    /* first file is the entry point */
    if (i == 0) project->entry = name;

    HashMapSet(&project->modules, name, result.value);
  }

  return result;
}

/* starting with the entry module, assemble a list of modules in order of dependency */
static Result ScanDependencies(Project *project)
{
  Val stack = Pair(project->entry, Nil, &project->mem);
  Val build_list = Nil;

  while (stack != Nil) {
    Val name, module, imports;

    /* get the next module in the stack */
    name = Head(stack, &project->mem);
    stack = Tail(stack, &project->mem);

    Assert(HashMapContains(&project->modules, name));
    module = HashMapGet(&project->modules, name);
    imports = ModuleImports(module, &project->mem);

    /* add current module to build list */
    build_list = Pair(module, build_list, &project->mem);

    /* add each of the module's imports to the stack */
    while (imports != Nil) {
      Val import = Head(imports, &project->mem);
      Val import_name = Tail(NodeExpr(import, &project->mem), &project->mem);

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->modules, import_name)) {
        char *filename = SymbolName(ModuleFile(module, &project->mem), &project->symbols);
        return ErrorResult("Missing module", filename, NodePos(import, &project->mem));
      }

      /* add import to the stack if it's not already in the build list */
      if (!ListContains(build_list, import_name, &project->mem)) {
        stack = Pair(import_name, stack, &project->mem);
      }

      imports = Tail(imports, &project->mem);
    }
  }

  return OkResult(build_list);
}

/* compiles a list of modules into a chunk that can be run */
static Result CompileProject(Val build_list, Chunk *chunk, Project *p)
{
  Compiler c;
  u32 i = 0, patch;
  u32 num_modules = ListLength(build_list, &p->mem);
  Val module_env = ExtendEnv(Nil, CompileEnv(&p->mem, &p->symbols), &p->mem);

  InitChunk(chunk);
  InitCompiler(&c, &p->mem, &p->symbols, &p->modules, chunk);

  /* env is {modules} -> {primitives} -> nil
     although modules are technically reachable at runtime in the environment,
     the compiler wouldn't be able to resolve them, so it would never compile
     access to them */
  module_env = ExtendEnv(module_env, MakeTuple(num_modules, &p->mem), &p->mem);
  PushByte(OpConst, 0, chunk);
  PushConst(IntVal(num_modules), 0, chunk);
  PushByte(OpTuple, 0, chunk);
  PushByte(OpExtend, 0, chunk);

  while (build_list != Nil) {
    Result result;
    char *module_name;
    Val module = Head(build_list, &p->mem);

    build_list = Tail(build_list, &p->mem);
    module_name = SymbolName(ModuleName(module, &p->mem), &p->symbols);

    printf("Compiling %s\n", module_name);

    /* copy filename symbol to chunk */
    Sym(SymbolName(ModuleFile(module, &p->mem), &p->symbols), &chunk->symbols);

    BeginChunkFile(ModuleFile(module, &p->mem), chunk);

    /* each module will create a lambda and define itself */
    result = CompileModule(module, module_name, module_env, i, &c);
    if (!result.ok) return result;

    EndChunkFile(chunk);

    /* modules can't reference themselves, so we define each module after */
    Define(ModuleName(module, &p->mem), i, module_env, &p->mem);
    i++;
  }

  patch = PushByte(OpLink, 0, chunk);
  PushByte(0, 0, chunk);
  PushByte(OpLookup, 0, chunk);
  PushConst(IntVal(0), 0, chunk);
  PushConst(IntVal(i-1), 0, chunk);
  PushByte(OpApply, 0, chunk);
  PushConst(IntVal(0), 0, chunk);
  PatchChunk(chunk, patch+1, IntVal(chunk->code.count - patch));
  PushByte(OpPop, 0, chunk);

  return OkResult(Nil);
}
