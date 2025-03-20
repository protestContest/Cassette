#include "compile/project.h"
#include "compile/compile.h"
#include "compile/parse.h"
#include "runtime/mem.h"
#include "runtime/ops.h"
#include "runtime/symbol.h"
#include "runtime/vm.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/vec.h"

static Error *FileNotFound(char *filename)
{
  Error *error = NewError("File \"^\" not found", filename, 0, 0);
  error->message = FormatString(error->message, filename);
  return error;
}

static Error *ModuleNotFound(char *name, char *file, u32 pos, u32 len)
{
  Error *error = NewError("Module \"^\" not found", file, pos, len);
  error->message = FormatString(error->message, name);
  return error;
}

static Error *BadImportList(char *imports)
{
  Error *error = NewError("Bad import list: \"^\"", 0, 0, 0);
  error->message = FormatString(error->message, imports);
  return error;
}

static void AddStrings(ASTNode *node, Program *program, HashMap *strings)
{
  u32 i;
  if (node->nodeType == symNode) {
    u32 len, sym;
    char *name;
    sym = RawVal(NodeValue(node));
    if (HashMapContains(strings, sym)) return;
    HashMapSet(strings, sym, 1);
    name = SymbolName(sym);
    len = StrLen(name);
    if (!program->strings) {
      program->strings = NewVec(u8, len);
    }
    GrowVec(program->strings, len);
    Copy(name, VecEnd(program->strings) - len, len);
    VecPush(program->strings, 0);
    return;
  }
  if (IsTerminal(node)) return;
  for (i = 0; i < NodeCount(node); i++) {
    AddStrings(NodeChild(node, i), program, strings);
  }
}

static void AddChunkSource(Chunk *chunk, char *filename, SourceMap *map)
{
  AddSourceFile(map, filename, ChunkSize(chunk));
  while (chunk) {
    u32 src = chunk->src;
    u32 count = VecCount(chunk->data);
    AddSourcePos(map, src, count);
    chunk = chunk->next;
  }
}

Project *NewProject(Opts *opts)
{
  Project *project = malloc(sizeof(Project));
  project->modules = 0;
  InitHashMap(&project->mod_map);
  project->program = 0;
  project->build_list = 0;
  project->opts = opts ? opts : DefaultOpts();
  project->entry_index = 0;
  return project;
}

void FreeProject(Project *project)
{
  u32 i;
  for (i = 0; i < VecCount(project->modules); i++) {
    DestroyModule(&project->modules[i]);
  }
  FreeVec(project->modules);
  DestroyHashMap(&project->mod_map);
  FreeVec(project->build_list);
  free(project);
}

Error *AddProjectFile(Project *project, char *filename)
{
  u32 i;
  Module mod;
  char *source;
  for (i = 0; i < VecCount(project->modules); i++) {
    if (StrEq(project->modules[i].filename, filename)) return 0;
  }
  source = ReadTextFile(filename);
  if (!source) return FileNotFound(filename);
  InitModule(&mod);
  mod.filename = NewString(filename);
  mod.source = source;
  VecPush(project->modules, mod);
  return 0;
}

void ScanProjectFolder(Project *project, char *path)
{
  u32 i;
  FileList *list = ListFiles(path, project->opts->source_ext, 0);

  for (i = 0; i < list->count; i++) {
    Error *error = AddProjectFile(project, list->filenames[i]);
    if (error) FreeError(error);
  }
  FreeFileList(list);
}

Error *ScanManifest(Project *project, char *path)
{
  Error *error;
  Cut cut = {0};
  char *cwd = DirName(path);
  char *source = ReadTextFile(path);
  if (!source) return FileNotFound(path);
  cut.tail = StrSpan(source, source + StrLen(source));
  while (cut.tail.len) {
    char *line;
    cut = StrCut(cut.tail, '\n');
    line = cut.head.data;
    line[cut.head.len] = 0;
    line = JoinStr(cwd, line, '/');
    error = AddProjectFile(project, line);
    free(line);
    if (error) break;
  }
  free(source);
  return error;
}

/* For each module, add it to the scan list. Then scan each of its imports.
 * Afterwards, move the module to the build list. Thus all of that module's
 * dependencies are in the build list before it. */
static Error *ScanModuleDeps(
  u32 mod_index,
  u32 **scan_list, /* vec */
  HashMap *build_set,
  HashMap *scan_set,
  Project *project)
{
  u32 i;
  Error *error;
  Module *mod = &project->modules[mod_index];
  ASTNode *imports = ModuleImports(mod);

  mod->id = VecCount(project->build_list) + VecCount(*scan_list);

  if (NodeCount(imports) > 0) {
    VecPush(*scan_list, mod_index);
    HashMapSet(scan_set, mod_index, 1);
    for (i = 0; i < NodeCount(imports); i++) {
      ASTNode *import = NodeChild(imports, i);
      u32 name = NodeValue(NodeChild(import, 0));
      u32 index;
      if (name == Symbol("Host")) continue; /* Skip host pseudo-module */
      if (!HashMapContains(&project->mod_map, name)) {
        return ModuleNotFound(SymbolName(name), mod->filename, import->start,
            import->end - import->start);
      }

      index = HashMapGet(&project->mod_map, name);
      if (HashMapContains(build_set, index)) continue;
      if (HashMapContains(scan_set, index)) continue;

      error = ScanModuleDeps(index, scan_list, build_set, scan_set, project);
      if (error) return error;
    }
    VecPop(*scan_list);
    HashMapDelete(scan_set, mod_index);
  }

  VecPush(project->build_list, mod_index);
  HashMapSet(build_set, mod_index, 1);
  return 0;
}

static Error *ScanDeps(Project *project)
{
  u32 *scan_list = 0; /* vec */
  HashMap build_set = EmptyHashMap;
  HashMap scan_set = EmptyHashMap;
  Error *error = ScanModuleDeps(project->entry_index, &scan_list, &build_set, &scan_set, project);
  FreeVec(scan_list);
  DestroyHashMap(&build_set);
  DestroyHashMap(&scan_set);
  return error;
}

/* Serializes the compiled modules into a program */
static void LinkModules(Project *project)
{
  u32 i;
  Chunk *intro_chunk = 0;
  u32 size;
  Program *program = NewProgram();
  HashMap strings = EmptyHashMap;
  u8 *cur;

  /* The intro chunk sets up the module register (if there are any imports) */
  if (VecCount(project->build_list) > 1) {
    intro_chunk = NewChunk(0);
    Emit(opTuple, intro_chunk);
    EmitInt(VecCount(project->build_list) - 1, intro_chunk);
    Emit(opPull, intro_chunk);
    EmitInt(regMod, intro_chunk);
  }

  /* calculate program size */
  size = intro_chunk ? ChunkSize(intro_chunk) : 0;
  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *mod = &project->modules[project->build_list[i]];
    size += ChunkSize(mod->code);
  }

  program->code = NewVec(u8, size);
  cur = program->code;

  /* serialize intro chunk */
  if (intro_chunk) {
    AddChunkSource(intro_chunk, 0, &program->srcmap);
    cur = SerializeChunk(intro_chunk, cur);
    FreeChunk(intro_chunk);
  }

  /* serialize each module chunk */
  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *mod = &project->modules[project->build_list[i]];
    AddChunkSource(mod->code, mod->filename, &program->srcmap);
    AddStrings(mod->ast, program, &strings);
    cur = SerializeChunk(mod->code, cur);
  }

  DestroyHashMap(&strings);
  RawVecCount(program->code) = size;
  project->program = program;
}

Error *BuildProject(Project *project)
{
  u32 i;
  Error *error;
  Compiler c;

  SetSymbolSize(valBits);

  /* parse each source file and add it to mod_map */
  for (i = 0; i < VecCount(project->modules); i++) {
    Module *mod = &project->modules[i];
    u32 name;

    mod->ast = ParseModuleHeader(mod->source);

    if (IsErrorNode(mod->ast)) {
      char *msg = SymbolName(mod->ast->data.value);
      u32 len = mod->ast->end - mod->ast->start;
      return NewError(msg, mod->filename, mod->ast->start, len);
    }

    name = NodeValue(ModuleName(mod));
    if (name) HashMapSet(&project->mod_map, name, i);
  }

  /* create the build list */
  error = ScanDeps(project);
  if (error) return error;

  /* parse each module in the build list */
  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 mod_index = project->build_list[i];
    Module *mod = &project->modules[mod_index];
    ASTNode *exports;
    u32 j;

    FreeNode(mod->ast);
    mod->ast = SimplifyNode(ParseModule(mod->source), 0);

    if (IsErrorNode(mod->ast)) {
      char *msg = SymbolName(mod->ast->data.value);
      u32 len = mod->ast->end - mod->ast->start;
      return NewError(msg, mod->filename, mod->ast->start, len);
    }

    /* index the module's exports */
    exports = ModuleExports(mod);
    for (j = 0; j < NodeCount(exports); j++) {
      u32 export = NodeValue(NodeChild(exports, j));
      HashMapSet(&mod->exports, export, j);
    }
  }

  /* compile each module in the build list */
  InitCompiler(&c, project);
  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 mod_index = project->build_list[i];
    Module *mod = &project->modules[mod_index];
    c.current_mod = mod_index;

    error = Compile(&c, mod);
    if (error) {
      DestroyCompiler(&c);
      return error;
    }
  }
  DestroyCompiler(&c);

  /* generate program from compiled modules */
  LinkModules(project);

  return 0;
}
