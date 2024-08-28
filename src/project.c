#include "project.h"
#include "compile.h"
#include "chunk.h"
#include "env.h"
#include "parse.h"
#include "primitives.h"
#include "univ/math.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"

static Result ModuleNotFound(char *name, char *file, u32 pos)
{
  u32 len = strlen(name);
  Error *error = NewError(NewString("Modyule \"^\" not found"), file, pos, len);
  error->message = FormatString(error->message, name);
  return Err(error);
}

static Result DuplicateModule(char *name, char *file)
{
  Error *error = NewError(NewString("Duplicate module \"^\""), file, 0, 0);
  error->message = FormatString(error->message, name);
  return Err(error);
}

static FileList *ListProjectFiles(char *entryfile, char *searchpath)
{
  u32 i;
  FileList *list = ListFiles(DirName(entryfile), ".ct", 0);
  if (searchpath) list = ListFiles(searchpath, ".ct", list);
  for (i = 1; i < list->count; i++) {
    if (StrEq(entryfile, list->filenames[i])) {
      char *tmp = list->filenames[i];
      list->filenames[i] = list->filenames[0];
      list->filenames[0] = tmp;
      break;
    }
  }
  return list;
}

Project *NewProject(char *entryfile, char *searchpath)
{
  u32 i;
  FileList *files;
  Project *project = malloc(sizeof(Project));
  files = ListProjectFiles(entryfile, searchpath);
  InitHashMap(&project->mod_map);
  project->modules = 0;
  project->build_list = 0;
  for (i = 0; i < files->count; i++) {
    Module mod;
    InitModule(&mod);
    mod.filename = files->filenames[i];
    mod.source = ReadFile(files->filenames[i]);
    VecPush(project->modules, mod);
  }
  free(files->filenames);
  free(files);
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

static Result ScanProjectDeps(Project *project)
{
  u32 i;
  Result result;
  HashMap scan_set = EmptyHashMap;
  u32 *scan_list = 0;

  for (i = 0; i < VecCount(project->modules); i++) {
    result = ParseModuleHeader(&project->modules[i]);
    if (IsError(result)) return result;
    if (HashMapContains(&project->mod_map, project->modules[i].name)) {
      if (project->modules[i].name == 0) continue;
      result = DuplicateModule(SymbolName(project->modules[i].name), project->modules[i].filename);
      return result;
    }
    HashMapSet(&project->mod_map, project->modules[i].name, i);
  }

  VecPush(scan_list, project->modules[0].name);

  while (VecCount(scan_list) > 0) {
    u32 modname = VecPop(scan_list);
    u32 modnum;
    Module *module;

    modnum = HashMapGet(&project->mod_map, modname);
    module = &project->modules[modnum];

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

  return Ok(project);
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

Result BuildProject(Project *project)
{
  Result result;
  u32 i;
  Env *env = 0;
  Program *program;
  HashMap symbols = EmptyHashMap;

  SetSymbolSize(valBits);

  result = ScanProjectDeps(project);
  if (IsError(result)) return result;

  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 modnum = project->build_list[i];
    Module *module = &project->modules[modnum];

    result = ParseModuleBody(module);
    if (IsError(result)) return result;

    CollectSymbols(module->ast, &symbols);
    AddSymbol(Symbol(module->filename), &symbols);

    result = CompileModule(module, project->modules, env);
    if (IsError(result)) return result;
  }

  program = NewProgram();
  LinkModules(project, program);
  program->strings = SerializeSymbols(&symbols);
  DestroyHashMap(&symbols);
  return Ok(program);
}
