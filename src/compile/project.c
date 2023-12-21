#include "project.h"
#include "compile.h"
#include "univ/str.h"
#include "parse.h"
#include "runtime/primitives.h"
#include "runtime/env.h"
#include "runtime/ops.h"
#include "univ/system.h"
#include "debug.h"
#include <stdio.h>

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
static void AddPrimitiveModules(Project *p);

Result BuildProject(u32 num_files, char **filenames, char *stdlib, Chunk *chunk)
{
  u32 i;
  Result result;
  Project project;

  InitProject(&project);

  for (i = 0; i < num_files; i++) {
    ObjVecPush(&project.manifest, CopyStr(filenames[i], StrLen(filenames[i])));
  }

  if (stdlib) {
    DirContents(stdlib, "ct", &project.manifest);
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
  InitMem(&p->mem, 1024*16, 0);
  InitSymbolTable(&p->symbols);
  DefineSymbols(&p->symbols);
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

    result = ParseModule(&parser, source);
    if (!result.ok) return result;

    Free(source);

    /* PrintAST(result.value, &parser); */

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
  HashMap seen;
  Val stack = Pair(project->entry, Nil, &project->mem);
  Val build_list = Nil;

  InitHashMap(&seen);

  AddPrimitiveModules(project);

  while (stack != Nil) {
    Val name, module, imports;

    /* get the next module in the stack */
    name = Head(stack, &project->mem);
    stack = Tail(stack, &project->mem);

    if (HashMapContains(&seen, name)) continue;

    Assert(HashMapContains(&project->modules, name));
    module = HashMapGet(&project->modules, name);

    /* skip nil modules (e.g. primitive modules) */
    if (ModuleBody(module, &project->mem) == Nil) continue;

    imports = ModuleImports(module, &project->mem);

    /* add current module to build list */
    build_list = Pair(module, build_list, &project->mem);
    HashMapSet(&seen, name, 1);

    /* add each of the module's imports to the stack */
    while (imports != Nil) {
      Val node = Head(imports, &project->mem);
      Val import = NodeExpr(node, &project->mem);
      Val import_name = Head(import, &project->mem);
      imports = Tail(imports, &project->mem);

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->modules, import_name)) {
        char *filename = SymbolName(ModuleFile(module, &project->mem), &project->symbols);
        return ErrorResult("Module not found", filename, NodePos(node, &project->mem));
      }

      /* add import to the stack if it's not already in the build list */
      if (!HashMapContains(&seen, import_name)) {
        stack = Pair(import_name, stack, &project->mem);
      }
    }
  }

  DestroyHashMap(&seen);

  return OkResult(build_list);
}

/* compiles a list of modules into a chunk that can be run */
static Result CompileProject(Val build_list, Chunk *chunk, Project *p)
{
  Compiler c;
  u32 i;
  u32 num_modules = ListLength(build_list, &p->mem) - 1; /* exclude entry script */
  Val compile_env;
  Result result = OkResult(Nil);

  if (build_list == Nil) return result;

  InitCompiler(&c, &p->mem, &p->symbols, &p->modules, chunk);

  result = CompileInitialEnv(num_modules, &c);
  if (!result.ok) return result;
  compile_env = result.value;

  /* pre-define all modules */
  for (i = 0; i < num_modules; i++) {
    Val module = ListGet(build_list, i, &p->mem);
    Define(ModuleName(module, &p->mem), i, compile_env, &p->mem);
  }

  /* compile each module except the entry mod */
  for (i = 0; i < num_modules; i++) {
    Val module = Head(build_list, &p->mem);
    build_list = Tail(build_list, &p->mem);

    c.env = compile_env;
    result = CompileModule(module, i, &c);
    if (!result.ok) return result;
  }

  /* compile the entry module a little differently */
  c.env = compile_env;
  result = CompileScript(Head(build_list, &p->mem), &c);

  return result;
}

Val MakeModule(Val name, Val body, Val imports, Val exports, Val filename, Mem *mem)
{
  Val module = MakeTuple(5, mem);
  TupleSet(module, 0, name, mem);
  TupleSet(module, 1, body, mem);
  TupleSet(module, 2, imports, mem);
  TupleSet(module, 3, exports, mem);
  TupleSet(module, 4, filename, mem);
  return module;
}

u32 CountExports(Val nodes, HashMap *modules, Mem *mem)
{
  u32 count = 0;
  while (nodes != Nil) {
    Val import = NodeExpr(Head(nodes, mem), mem);
    Val mod_name = Head(import, mem);
    Val alias = Tail(import, mem);

    if (alias != Nil) {
      count++;
    } else if (HashMapContains(modules, mod_name)) {
      Val module = HashMapGet(modules, mod_name);
      count += ListLength(ModuleExports(module, mem), mem);
    }
    nodes = Tail(nodes, mem);
  }
  return count;
}

static void AddPrimitiveModules(Project *p)
{
  /* set primitive modules in the module map */
  PrimitiveModuleDef *primitives = GetPrimitives();
  u32 num_primitives = NumPrimitives();
  u32 i, j;

  for (i = 0; i < num_primitives; i++) {
    Val mod, exports = Nil;
    for (j = 0; j < primitives[i].num_fns; j++) {
      exports = Pair(primitives[i].fns[j].name, exports, &p->mem);
    }
    exports = ReverseList(exports, Nil, &p->mem);
    mod = MakeModule(primitives[i].module, Nil, Nil, exports, Nil, &p->mem);
    HashMapSet(&p->modules, primitives[i].module, mod);
  }
}
