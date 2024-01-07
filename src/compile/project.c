#include "project.h"
#include "runtime/chunk.h"
#include "compile.h"
#include "univ/str.h"
#include "parse.h"
#include "runtime/primitives.h"
#include "env.h"
#include "runtime/ops.h"
#include "univ/system.h"
#include "debug.h"
#include <stdio.h>

typedef struct {
  Options opts;
  ObjVec modules;
  HashMap mod_map;
  SymbolTable symbols;
  ObjVec manifest;
  ObjVec build_list;
} Project;

static void InitProject(Project *project, Options opts);
static void DestroyProject(Project *project);
static Result ReadManifest(char *filename, Project *project);
static Result ParseModules(Project *project);
static Result ScanDependencies(Project *project);
static Result CompileProject(Project *project);

Result BuildProject(Options opts)
{
  u32 i;
  Result result;
  Project project;

  InitProject(&project, opts);

  for (i = 0; i < opts.num_files; i++) {
    ObjVecPush(&project.manifest, CopyStr(opts.filenames[i], StrLen(opts.filenames[i])));
  }

  if (opts.stdlib_path) {
    DirContents(opts.stdlib_path, "ct", &project.manifest);
  }

  result = ParseModules(&project);
  if (result.ok) result = ScanDependencies(&project);
  if (result.ok) result = CompileProject(&project);

  DestroyProject(&project);

  return result;
}

static void InitProject(Project *project, Options opts)
{
  project->opts = opts;
  InitVec((Vec*)&project->modules, sizeof(Node*), 8);
  InitHashMap(&project->mod_map);
  InitSymbolTable(&project->symbols);
  InitVec((Vec*)&project->manifest, sizeof(char*), 8);
  InitVec((Vec*)&project->build_list, sizeof(Node*), 8);

  if (opts.debug) {
    DefineSymbols(&project->symbols);
  }
}

static void DestroyProject(Project *project)
{
  u32 i;
  for (i = 0; i < project->modules.count; i++) {
    FreeAST(VecRef(&project->modules, i));
  }
  DestroyVec((Vec*)&project->modules);
  DestroyHashMap(&project->mod_map);
  DestroySymbolTable(&project->symbols);
  for (i = 0; i < project->manifest.count; i++) {
    Free(project->manifest.items[i]);
  }
  DestroyVec((Vec*)&project->manifest);
  DestroyVec((Vec*)&project->build_list);
}

static Result ParseModules(Project *project)
{
  Result result;
  u32 i;

  for (i = 0; i < project->manifest.count; i++) {
    char *filename = project->manifest.items[i];
    result = ParseFile(filename, &project->symbols);
    if (!result.ok) return result;

    if (project->opts.debug) {
      PrintAST(result.data, &project->symbols);
    }

    HashMapSet(&project->mod_map, ModuleName(result.data), project->modules.count);
    ObjVecPush(&project->modules, result.data);
  }

  return result;
}

/* starting with the entry module, assemble a list of modules in order of dependency */
static Result ScanDependencies(Project *project)
{
  IntVec stack;
  HashMap seen;
  u32 i;

  InitVec((Vec*)&stack, sizeof(u32), 8);
  IntVecPush(&stack, ModuleName(VecRef(&project->modules, 0)));
  InitHashMap(&seen);

  while (stack.count > 0) {
    Val name = VecPop(&stack);
    Node *module, *imports;
    if (HashMapContains(&seen, name)) continue;

    Assert(HashMapContains(&project->mod_map, name));
    module = VecRef(&project->modules, HashMapGet(&project->mod_map, name));

    /* add current module to build list */
    ObjVecPush(&project->build_list, module);
    HashMapSet(&seen, name, 1);

    /* add each of the module's imports to the stack */
    imports = ModuleImports(module);
    for (i = 0; i < NumNodeChildren(imports); i++) {
      Node *import = NodeChild(imports, i);
      Val import_name = NodeValue(NodeChild(import, 0));

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->mod_map, import_name)) {
        char *filename = SymbolName(ModuleFile(module), &project->symbols);
        DestroyHashMap(&seen);
        DestroyVec((Vec*)&stack);
        return ErrorResult("Module not found", filename, import->pos);
      }

      /* add import to the stack if it's not already in the build list */
      if (!HashMapContains(&seen, import_name)) {
        IntVecPush(&stack, import_name);
      }
    }
  }

  DestroyHashMap(&seen);
  DestroyVec((Vec*)&stack);

  return OkResult(Nil);
}

/* compiles a list of modules into a chunk that can be run */
static Result CompileProject(Project *project)
{
  u32 i;
  Result result;
  Compiler c;
  u32 num_modules = project->build_list.count - 1; /* exclude entry script */
  Frame *compile_env = CompileEnv(num_modules);
  Chunk *chunk = Alloc(sizeof(Chunk));

  InitChunk(chunk);
  InitCompiler(&c, &project->symbols, &project->modules, &project->mod_map, chunk);

  CompileModuleFrame(num_modules, &c);

  /* pre-define all modules */
  for (i = 0; i < num_modules; i++) {
    Node *module = VecRef(&project->build_list, num_modules - i);
    FrameSet(compile_env, i, ModuleName(module));
  }

  /* compile each module except the entry mod */
  c.env = compile_env;
  for (i = 0; i < num_modules; i++) {
    Node *module = VecRef(&project->build_list, num_modules - i);
    Assert(c.env == compile_env);
    result = CompileModule(module, i, &c);
    if (!result.ok) break;
  }

  if (result.ok) {
    /* compile the entry module a little differently */
    Assert(c.env == compile_env);
    result = CompileScript(VecRef(&project->build_list, 0), &c);
  }

  if (result.ok) {
    result.data = chunk;
    return result;
  } else {
    DestroyChunk(chunk);
    FreeEnv(compile_env);
    return result;
  }
}
