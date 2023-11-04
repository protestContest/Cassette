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
  ObjVec manifest;
} Project;

static void InitProject(Project *p);
static void DestroyProject(Project *p);
static BuildResult ReadManifest(char *filename, Project *p);
static BuildResult ParseModules(Project *project);
static BuildResult ScanDependencies(Project *project);
static BuildResult CompileProject(Val build_list, Chunk *chunk, Project *p);

BuildResult BuildProject(char *manifest, Chunk *chunk)
{
  Project p;
  BuildResult result;

  InitProject(&p);

  result = ReadManifest(manifest, &p);
  if (!result.ok) return result;

  result = ParseModules(&p);
  if (!result.ok) return result;

  result = ScanDependencies(&p);
  if (!result.ok) return result;

  result = CompileProject(result.value, chunk, &p);
  if (!result.ok) return result;

  DestroyProject(&p);
  return BuildOk(Nil);
}

static void InitProject(Project *p)
{
  p->entry = Nil;
  InitHashMap(&p->modules);
  InitMem(&p->mem, 1024);
  InitSymbolTable(&p->symbols);
  InitVec((Vec*)&p->manifest, sizeof(char*), 8);
}

static void DestroyProject(Project *p)
{
  u32 i;
  DestroyHashMap(&p->modules);
  DestroyMem(&p->mem);
  DestroySymbolTable(&p->symbols);
  for (i = 0; i < p->manifest.count; i++) free(p->manifest.items[i]);
  DestroyVec((Vec*)&p->manifest);
  p->entry = Nil;
}

static BuildResult ReadManifest(char *filename, Project *p)
{
  char *source;
  char *cur;
  char *basename = Basename(filename, '/');

  source = ReadFile(filename);
  if (source == 0) return BuildError("Could not read manifest", filename, 0, 0);

  /* count lines */
  cur = SkipBlankLines(source);
  while (*cur != 0) {
    char *end = LineEnd(cur);
    if (*end == '\n') {
      *end = 0;
      end++;
    }
    ObjVecPush(&p->manifest, JoinStr(basename, cur, '/'));
    cur = end;

    cur = SkipBlankLines(cur);
  }

  free(basename);

  return BuildOk(Nil);
}

static BuildResult ParseModules(Project *project)
{
  u32 i;
  Parser p;
  BuildResult result = {true, 0, 0, 0, 0, 0};

  InitParser(&p, &project->mem, &project->symbols);

  /* parse each module and save in the module map */
  for (i = 0; i < project->manifest.count; i++) {
    Val name;
    result = ParseModule(project->manifest.items[i], &p);
    if (!result.ok) return result;

    name = ModuleName(result.value, &project->mem);

    /* first file is the entry point */
    if (i == 0) {
      project->entry = name;
    }

    HashMapSet(&project->modules, name, result.value);
  }

  return result;
}

static BuildResult ScanDependencies(Project *project)
{
  Val queue = Pair(project->entry, Nil, &project->mem), build_list = Nil;

  while (queue != Nil) {
    Val cur = Head(queue, &project->mem);
    Val module, imports;

    Assert(HashMapContains(&project->modules, cur));
    module = HashMapGet(&project->modules, cur);
    imports = ModuleImports(module, &project->mem);

    /* add current module to build list */
    build_list = Pair(module, build_list, &project->mem);

    queue = Tail(queue, &project->mem);
    while (imports != Nil) {
      Val import = Head(imports, &project->mem);
      Val import_name = Tail(NodeExpr(import, &project->mem), &project->mem);

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->modules, import_name)) {
        char *filename = ModuleFile(module, &project->mem, &project->symbols);
        return BuildError("Missing module", filename, 0, NodePos(import, &project->mem));
      }

      /* add import to the queue of modules to scan */
      if (!ListContains(build_list, import_name, &project->mem)) {
        queue = Pair(import_name, queue, &project->mem);
      }
      imports = Tail(imports, &project->mem);
    }
  }

  return BuildOk(build_list);
}

static BuildResult CompileProject(Val build_list, Chunk *chunk, Project *p)
{
  Compiler c;
  Val module_env = CompileEnv(&p->mem, &p->symbols);
  u32 i = 0, patch;
  u32 num_modules = ListLength(build_list, &p->mem);

  InitChunk(chunk);
  InitCompiler(&c, &p->mem, &p->symbols, &p->modules, chunk);

  /* env is {modules} -> {primitives} -> nil
     although modules are technically reachable at runtime in the environment,
     the compiler wouldn't be able to resolve them, and so would never compile
     access to them */
  module_env = ExtendEnv(module_env, MakeTuple(num_modules, &p->mem), &p->mem);
  PushByte(OpConst, 0, chunk);
  PushConst(IntVal(num_modules), 0, chunk);
  PushByte(OpTuple, 0, chunk);
  PushByte(OpExtend, 0, chunk);

  while (build_list != Nil) {
    BuildResult result;
    Val module = Head(build_list, &p->mem);
    build_list = Tail(build_list, &p->mem);

    printf("Compiling %s\n", SymbolName(ModuleName(module, &p->mem), &p->symbols));

    /* each module will create a lambda and define itself */
    result = CompileModule(module, module_env, i, &c);
    if (!result.ok) return result;

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

  return BuildOk(Nil);
}
