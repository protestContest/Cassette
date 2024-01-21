/*
A project is a list of files that are compiled into a chunk, which can be run by
the VM. It follows this process:

- Parse files: Each file is parsed, producing a ModuleNode. Each module has a
  name, a list of import nodes, a body, and a filename node. The entry script
  module name is always "*main*".
- Scan dependencies: Only modules which are imported in the project are compiled
  into a chunk. Starting with the main module, each module in a scan queue is
  added to the build list, and its imports are added to the scan queue. The
  build list includes all imported modules in order of dependency.
- Each module in the build list is compiled into the chunk. The chunk's high-
  level format is:
    - If there are imported modules, extend the env with a frame for module
      definitions.
    - Each non-main module is compiled as a lambda, which is then defined in the
      module frame.
    - The main module is compiled, not as a lambda, ending with "halt".
*/

#include "project.h"
#include "compile.h"
#include "debug.h"
#include "env.h"
#include "parse.h"
#include "runtime/chunk.h"
#include "runtime/ops.h"
#include "runtime/primitives.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/system.h"

typedef struct {
  Options opts;         /* CLI options */
  ObjVec modules;       /* List of module ASTs */
  HashMap mod_map;      /* Map from module name to index */
  SymbolTable symbols;  /* Table for parsed symbols and filenames */
  ObjVec manifest;      /* List of source files */
  ObjVec build_list;    /* List of module ASTs to compile */
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

  /* Copy project filenames into manifest */
  for (i = 0; i < opts.num_files; i++) {
    char *filename = CopyStr(opts.filenames[i], StrLen(opts.filenames[i]));
    ObjVecPush(&project.manifest, filename);
  }

  /* Copy stdlib module filenames into manifest */
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
  InitVec(&project->modules, sizeof(Node*), 8);
  InitHashMap(&project->mod_map);
  InitSymbolTable(&project->symbols);
  InitVec(&project->manifest, sizeof(char*), 8);
  InitVec(&project->build_list, sizeof(Node*), 8);
}

static void DestroyProject(Project *project)
{
  u32 i;
  for (i = 0; i < project->modules.count; i++) {
    FreeAST(VecRef(&project->modules, i));
  }
  DestroyVec(&project->modules);
  DestroyHashMap(&project->mod_map);
  DestroySymbolTable(&project->symbols);
  for (i = 0; i < project->manifest.count; i++) {
    Free(project->manifest.items[i]);
  }
  DestroyVec(&project->manifest);
  DestroyVec(&project->build_list);
}

static Result ParseModules(Project *project)
{
  Result result;
  u32 i;

  for (i = 0; i < project->manifest.count; i++) {
    char *filename = project->manifest.items[i];
    Node *module;

    result = ParseFile(filename, &project->symbols);
    if (!result.ok) return result;
    module = ResultItem(result);

    if (project->opts.debug) PrintAST(module, &project->symbols);

    /* the entry module is always named "*main*" */
    if (i == 0) {
      ModuleName(module) = MainModule;
      HashMapSet(&project->mod_map, ModuleName(module), project->modules.count);
      ObjVecPush(&project->modules, module);
    } else {
      /* skip unnamed modules */
      if (ModuleName(module) == MainModule) continue;

      /* return an error for duplicate modules */
      if (HashMapContains(&project->mod_map, ModuleName(module))) {
        char *filename = SymbolName(ModuleFile(module), &project->symbols);
        return ErrorResult("Duplicate module definition", filename, 0);
      }

      HashMapSet(&project->mod_map, ModuleName(module), project->modules.count);
      ObjVecPush(&project->modules, module);
    }
  }

  return result;
}

static Result ScanDependencies(Project *project)
{
  IntVec queue;
  HashMap seen;
  u32 cur, i;

  /* stack of modules to scan */
  InitVec(&queue, sizeof(u32), 1);
  InitHashMap(&seen);

  /* start with the entry file */
  IntVecPush(&queue, MainModule);

  cur = 0;
  while (cur < queue.count) {
    Val name = VecRef(&queue, cur++);
    Node *module, *imports;

    module = VecRef(&project->modules, HashMapGet(&project->mod_map, name));

    /* add current module to build list */
    ObjVecPush(&project->build_list, module);

    /* add each of the module's imports to the stack */
    imports = ModuleImports(module);
    for (i = 0; i < NumNodeChildren(imports); i++) {
      Node *import = NodeChild(imports, i);
      Val import_name = NodeValue(NodeChild(import, 0));

      /* skip modules we've already seen */
      if (HashMapContains(&seen, import_name)) continue;

      /* make sure we know about the imported module */
      if (!HashMapContains(&project->mod_map, import_name)) {
        char *filename = SymbolName(ModuleFile(module), &project->symbols);
        DestroyHashMap(&seen);
        DestroyVec(&queue);
        return ErrorResult("Module not found", filename, import->pos);
      }

      IntVecPush(&queue, import_name);
      HashMapSet(&seen, name, 1);
    }
  }

  DestroyHashMap(&seen);
  DestroyVec(&queue);

  return OkResult();
}

static Result CompileProject(Project *project)
{
  u32 i;
  Result result;
  Compiler c;
  u32 num_modules = project->build_list.count;
  Chunk *chunk = Alloc(sizeof(Chunk));

  InitChunk(chunk);
  InitCompiler(&c, &project->symbols, &project->modules,
               &project->mod_map, chunk);

  /* set up env and pre-define all modules */
  c.env = CompileEnv(num_modules);
  if (num_modules > 1) {
    CompileModuleFrame(num_modules - 1, &c);
    for (i = 0; i < num_modules - 1; i++) {
      Node *module = VecRef(&project->build_list, num_modules - 1 - i);
      FrameSet(c.env, i, ModuleName(module));
    }
  }

  /* compile each module */
  for (i = 0; i < num_modules; i++) {
    Node *module = VecRef(&project->build_list, num_modules - 1 - i);
    result = CompileModule(module, &c);
    if (!result.ok) break;
  }

  if (result.ok) {
    ResultItem(result) = chunk;
    return result;
  } else {
    DestroyChunk(chunk);
    FreeEnv(c.env);
    return result;
  }
}
