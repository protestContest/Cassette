#include "project.h"
#include "compile.h"
#include "chunk.h"
#include "mem.h"
#include "ops.h"
#include "parse.h"
#include "program.h"
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

static Error *ModuleNotFound(char *name, char *file, u32 pos)
{
  u32 len = strlen(name);
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

Project *NewProject(void)
{
  Project *project = malloc(sizeof(Project));
  InitHashMap(&project->mod_map);
  project->modules = 0;
  project->build_list = 0;
  return project;
}

void FreeProject(Project *project)
{
  u32 i;
  for (i = 0; i < VecCount(project->modules); i++) {
    DestroyModule(&project->modules[i]);
  }
  FreeVec(project->modules);
  if (project->build_list) FreeVec(project->build_list);
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

static Error *ScanProjectDeps(Project *project)
{
  u32 i;
  HashMap scan_set = EmptyHashMap;
  u32 *scan_list = 0;

  /* build hash map of all modules */
  for (i = 0; i < VecCount(project->modules); i++) {
    if (HashMapContains(&project->mod_map, project->modules[i].name)) {
      if (project->modules[i].name == 0) continue;
      return DuplicateModule(SymbolName(project->modules[i].name), project->modules[i].filename);
    }
    HashMapSet(&project->mod_map, project->modules[i].name, i);
  }

  /* start scanning with the entry module */
  VecPush(scan_list, project->modules[0].name);
  HashMapSet(&scan_set, project->modules[0].name, 1);

  while (VecCount(scan_list) > 0) {
    u32 modname = VecPop(scan_list);
    u32 modnum = HashMapGet(&project->mod_map, modname);
    Module *module = &project->modules[modnum];

    VecPush(project->build_list, modnum);

    for (i = 0; i < VecCount(module->imports); i++) {
      u32 import = module->imports[i].module;
      if (!HashMapContains(&scan_set, import)) {
        if (!HashMapContains(&project->mod_map, import)) {
          DestroyHashMap(&scan_set);
          FreeVec(scan_list);
          return ModuleNotFound(SymbolName(import), module->filename, module->imports[i].pos);
        }

        HashMapSet(&scan_set, import, 1);
        VecPush(scan_list, import);
      }
    }
  }

  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 modnum = project->build_list[i];
    Module *module = &project->modules[modnum];
    module->id = i;
  }

  DestroyHashMap(&scan_set);
  FreeVec(scan_list);

  return 0;
}

static void LinkModules(Project *project, Program *program)
{
  u32 i;
  u32 size = 0;
  u8 *data, *cur;
  u32 num_modules = VecCount(project->build_list);

  Chunk *intro = CompileIntro(num_modules);
  Chunk *outro = NewChunk(0);
  ChunkWrite(opSetMod, outro);
  outro = AppendChunk(outro, CompileCallMod(0));
  ChunkWrite(opDrop, outro);
  ChunkWrite(opHalt, outro);
  size = ChunkSize(intro) + ChunkSize(outro);
  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 idx = project->build_list[i];
    size += ChunkSize(project->modules[idx].code);
  }
  data = NewVec(u8, size);
  RawVecCount(data) = size;
  cur = data;

  AddChunkSource(intro, 0, &program->srcmap);
  cur = SerializeChunk(intro, cur);
  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 modnum = project->build_list[num_modules - 1 - i];
    Module *mod = &project->modules[modnum];
    AddChunkSource(mod->code, mod->filename, &program->srcmap);
    cur = SerializeChunk(mod->code, cur);
  }
  AddChunkSource(outro, 0, &program->srcmap);
  SerializeChunk(outro, cur);

  FreeChunk(intro);
  FreeChunk(outro);

  program->code = data;
}

static void CollectSymbols(ASTNode *node, HashMap *symbols)
{
  if (node->type == strNode) {
    HashMapSet(symbols, RawVal(node->data.value), 1);
  }

  if (!IsTerminal(node)) {
    u32 i;
    for (i = 0; i < VecCount(node->data.children); i++) {
      CollectSymbols(node->data.children[i], symbols);
    }
  }
}

static void AddSymbol(u32 symbol, HashMap *symbols)
{
  HashMapSet(symbols, symbol, 1);
}

static char *SerializeSymbols(HashMap *symbols)
{
  char *data = 0;
  u32 i;
  for (i = 0; i < symbols->count; i++) {
    u32 sym = HashMapKey(symbols, i);
    char *name = SymbolName(sym);
    u32 len = strlen(name);
    GrowVec(data, len);
    Copy(name, VecEnd(data) - len, len);
    VecPush(data, 0);
  }

  return data;
}

Error *BuildProject(Project *project)
{
  u32 i;

  SetSymbolSize(valBits);

  for (i = 0; i < VecCount(project->modules); i++) {
    Module *mod = &project->modules[i];
    mod->ast = ParseModule(mod->source);
    if (IsErrorNode(mod->ast)) {
      char *msg = SymbolName(mod->ast->data.value);
      u32 len = mod->ast->end - mod->ast->start;
      return NewError(msg, mod->filename, mod->ast->start, len);
    }

    PrintNode(mod->ast);
  }

  return 0;
}
