#include "project.h"
#include "compile.h"
#include "univ/str.h"
#include "parse.h"
#include "runtime/primitives.h"
#include "module.h"
#include "runtime/env.h"
#include "runtime/ops.h"
#include "univ/system.h"

typedef struct {
  Val entry;
  HashMap modules;
  Mem mem;
  SymbolTable symbols;
  ObjVec manifest;
} Project;

static void InitProject(Project *p);
static void DestroyProject(Project *p);
static Result ReadManifest(char *filename, Project *project);
static Result ParseModules(Project *project);
static Result ScanDependencies(Project *project);
static Result CompileProject(Val build_list, Chunk *chunk, Project *p);

Result BuildProject(char *manifest, Chunk *chunk)
{
  Result result;
  Project project;

  InitProject(&project);

  result = ReadManifest(manifest, &project);
  if (result.ok) result = ParseModules(&project);
  if (result.ok) result = ScanDependencies(&project);
  if (result.ok) result = CompileProject(result.value, chunk, &project);

  DestroyProject(&project);
  return result;
}

Result BuildScripts(u32 num_files, char **filenames, Chunk *chunk)
{
  u32 i;
  Result result;
  Project project;

  InitProject(&project);
  for (i = 0; i < num_files; i++) {
    ObjVecPush(&project.manifest, CopyStr(filenames[i]));
  }

  result = ParseModules(&project);
  if (result.ok) result = ScanDependencies(&project);
  if (result.ok) result = CompileProject(result.value, chunk, &project);

  DestroyProject(&project);

  return result;
}

static void InitProject(Project *p)
{
  p->entry = Nil;
  InitVec((Vec*)&p->manifest, sizeof(char*), 8);
  InitHashMap(&p->modules);
  InitMem(&p->mem, 1024);
  InitSymbolTable(&p->symbols);
}

static void DestroyProject(Project *p)
{
  u32 i;
  DestroyHashMap(&p->modules);
  DestroyMem(&p->mem);
  DestroySymbolTable(&p->symbols);
  for (i = 0; i < p->manifest.count; i++) {
    Free(p->manifest.items[i]);
  }
  DestroyVec((Vec*)&p->manifest);
  p->entry = Nil;
}

Result ReadManifest(char *filename, Project *project)
{
  char *source;
  char *cur;
  char *basename = Basename(filename, '/');

  source = ReadFile(filename);
  if (source == 0) return ErrorResult("Could not read manifest", filename, 0);

  cur = SkipBlankLines(source);
  while (*cur != 0) {
    char *end = LineEnd(cur);
    if (*end == '\n') {
      /* replace newline with string terminator */
      *end = 0;
      end++;
    }
    /* add full path filename */
    ObjVecPush(&project->manifest, JoinStr(basename, cur, '/'));
    cur = end;

    /* skip to next non-space character */
    cur = SkipBlankLines(cur);
  }

  Free(source);
  Free(basename);

  return OkResult(Nil);
}

/* parses each file and puts the result in a hashmap, keyed by module name (or
   filename if it isn't a module) */
static Result ParseModules(Project *project)
{
  Result result;
  Parser parser;
  u32 i;

  InitParser(&parser, &project->mem, &project->symbols);

  for (i = 0; i < project->manifest.count; i++) {
    Val name;
    char *filename = project->manifest.items[i];
    char *source = ReadFile(filename);

    if (source == 0) return ErrorResult("Could not read file", filename, 0);
    parser.filename = filename;
    InitLexer(&parser.lex, source, 0);

    result = ParseModule(&parser);
    if (!result.ok) return result;

    Free(source);

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
      Val import_name = NodeExpr(import, &project->mem);

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->modules, import_name)) {
        char *filename = SymbolName(ModuleFile(module, &project->mem), &project->symbols);
        return ErrorResult("Module not found", filename, NodePos(import, &project->mem));
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
  u32 i;
  u32 num_modules = ListLength(build_list, &p->mem);
  Val module_env = ExtendEnv(Nil, CompileEnv(&p->mem), &p->mem);

  InitChunk(chunk);
  InitCompiler(&c, &p->mem, &p->symbols, &p->modules, chunk);

  /* env is {modules} -> {primitives} -> nil
     although modules are technically reachable at runtime in the environment,
     the compiler wouldn't be able to resolve them, so it would never compile
     access to them */
  if (num_modules > 1) {
    module_env = ExtendEnv(module_env, MakeTuple(num_modules - 1, &p->mem), &p->mem);
    PushByte(OpConst, 0, chunk);
    PushConst(IntVal(num_modules - 1), 0, chunk);
    PushByte(OpTuple, 0, chunk);
    PushByte(OpExtend, 0, chunk);
  }

  for (i = 0; i < num_modules; i++) {
    Result result;
    Val module = Head(build_list, &p->mem);
    build_list = Tail(build_list, &p->mem);

    c.env = module_env;
    if (i == num_modules - 1) {
      result = CompileScript(module, &c);
    } else {
      result = CompileModule(module, i, &c);
      Define(ModuleName(module, &p->mem), i, module_env, &p->mem);
    }
    if (!result.ok) return result;
  }

  /* clean up module env */
  if (num_modules > 1) {
    PushByte(OpExport, 0, chunk);
    PushByte(OpPop, 0, chunk);
  }

  return OkResult(Nil);
}
