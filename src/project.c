#include "project.h"
#include "compile.h"
#include "mem.h"
#include "ops.h"
#include "parse.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"

static Error *FileNotFound(char *filename)
{
  Error *error = NewError(NewString("File \"^\" not found"), filename, 0, 0);
  error->message = FormatString(error->message, filename);
  return error;
}

static Error *ModuleNotFound(char *name, char *file, u32 pos, u32 len)
{
  Error *error = NewError(NewString("Module \"^\" not found"), file, pos, len);
  error->message = FormatString(error->message, name);
  return error;
}

static Error *DuplicateModule(char *name, char *file)
{
  Error *error = NewError(NewString("Duplicate module \"^\""), file, 0, 0);
  error->message = FormatString(error->message, name);
  return error;
}

static Error *CircularDependency(char *mod1, char *mod2, char *file)
{
  Error *error = NewError(NewString("Circular dependency between ^ and ^"), file, 0, 0);
  error->message = FormatString(error->message, mod1);
  error->message = FormatString(error->message, mod2);
  return error;
}

Project *NewProject(void)
{
  Project *project = malloc(sizeof(Project));
  InitHashMap(&project->mod_map);
  project->modules = 0;
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
  source = ReadFile(filename);
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
  FileList *list = ListFiles(path, ".ct", 0);

  for (i = 0; i < list->count; i++) {
    Error *error = AddProjectFile(project, list->filenames[i]);
    if (error) FreeError(error);
  }
  FreeFileList(list);
}

static Error *ScanModuleDeps(
  u32 mod_index,
  u32 **scan_list,
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
      if (!HashMapContains(&project->mod_map, name)) {
        return ModuleNotFound(SymbolName(name), mod->filename, import->start, import->end - import->start);
      }

      index = HashMapGet(&project->mod_map, name);
      if (HashMapContains(build_set, index)) continue;
      if (HashMapContains(scan_set, index)) {
        char *mod_name = SymbolName(NodeValue(ModuleName(mod)));
        return CircularDependency(mod_name, SymbolName(name), mod->filename);
      }

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
  u32 *scan_list = 0;
  HashMap build_set = EmptyHashMap;
  HashMap scan_set = EmptyHashMap;
  Error *error = ScanModuleDeps(0, &scan_list, &build_set, &scan_set, project);
  FreeVec(scan_list);
  DestroyHashMap(&build_set);
  DestroyHashMap(&scan_set);
  return error;
}

static void LinkModules(Project *project)
{
  u32 i;
  Chunk *chunk = NewChunk(0);
  u32 size;
  Program *program = NewProgram();
  HashMap strings = EmptyHashMap;

  if (VecCount(project->build_list) > 1) {
    ChunkWrite(opTuple, chunk);
    ChunkWriteInt(VecCount(project->build_list) - 1, chunk);
    ChunkWrite(opSetMod, chunk);
    AddChunkSource(chunk, 0, &program->srcmap);
  }

  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *mod = &project->modules[project->build_list[i]];
    AddChunkSource(mod->code, mod->filename, &program->srcmap);
    AddStrings(mod->ast, program, &strings);
    chunk = AppendChunk(chunk, mod->code);
  }
  ChunkWrite(opHalt, chunk);
  DestroyHashMap(&strings);
  size = ChunkSize(chunk);
  program->code = NewVec(u8, size);
  SerializeChunk(chunk, program->code);
  RawVecCount(program->code) = size;
  project->program = program;
}

Error *BuildProject(Project *project)
{
  u32 i;
  Error *error;
  Compiler c;

  SetSymbolSize(valBits);

  for (i = 0; i < VecCount(project->modules); i++) {
    Module *mod = &project->modules[i];
    u32 name, j;
    ASTNode *exports;
    mod->ast = ParseModule(mod->source);
    if (IsErrorNode(mod->ast)) {
      char *msg = SymbolName(mod->ast->data.value);
      u32 len = mod->ast->end - mod->ast->start;
      return NewError(msg, mod->filename, mod->ast->start, len);
    }
    name = NodeValue(ModuleName(mod));
    if (name) HashMapSet(&project->mod_map, name, i);
    exports = ModuleExports(mod);
    for (j = 0; j < NodeCount(exports); j++) {
      u32 export = NodeValue(NodeChild(exports, j));
      HashMapSet(&mod->exports, export, j);
    }
    /* PrintNode(mod->ast); */
  }

  error = ScanDeps(project);
  if (error) return error;

  InitCompiler(&c, project->modules, &project->mod_map);
  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *mod = &project->modules[project->build_list[i]];
    c.mod_id = mod->id;
    mod->code = Compile(mod->ast, &c);
    if (c.error) {
      c.error->filename = NewString(mod->filename);
      return c.error;
    }

    /*
    printf("%s\n", SymbolName(NodeValue(ModuleName(mod))));
    DisassembleChunk(mod->code);
    */
  }
  DestroyCompiler(&c);

  LinkModules(project);

  return 0;
}
