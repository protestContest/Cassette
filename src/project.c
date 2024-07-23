#include "project.h"
#include "compile.h"
#include "chunk.h"
#include "env.h"
#include "parse.h"
#include "primitives.h"
#include <univ/file.h>
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/vec.h>

FileList *ListProjectFiles(char *entryfile, char *searchpath)
{
  u32 i;
  FileList *list = ListFiles(DirName(entryfile), ".ct", 0);
  if (searchpath) list = ListFiles(searchpath, ".ct", list);
  for (i = 0; i < list->count; i++) {
    if (i > 0 && StrEq(entryfile, list->filenames[i])) {
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
  InitHashMap(&project->file_map);
  project->modules = 0;
  project->build_list = 0;
  for (i = 0; i < files->count; i++) {
    Module mod;
    InitModule(&mod);
    mod.filename = files->filenames[i];
    mod.source = ReadFile(files->filenames[i]);
    VecPush(project->modules, mod);
  }
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
  DestroyHashMap(&project->file_map);
  free(project);
}

Result ScanProjectDeps(Project *project)
{
  u32 i;
  Result result;
  HashMap scan_set = EmptyHashMap;
  u32 *scan_list = 0;

  for (i = 0; i < VecCount(project->modules); i++) {
    result = ParseModuleHeader(&project->modules[i]);
    if (IsError(result)) return result;
    if (HashMapContains(&project->file_map, project->modules[i].name)) {
      return Error(duplicateModule, 0);
    }
    HashMapSet(&project->file_map, project->modules[i].name, i);
  }

  VecPush(scan_list, project->modules[0].name);

  while (VecCount(scan_list) > 0) {
    u32 modname = VecPop(scan_list);
    u32 modnum;
    Module *module;

    if (!HashMapContains(&project->file_map, modname)) {
      return Error(moduleNotFound, 0);
    }
    modnum = HashMapGet(&project->file_map, modname);
    module = &project->modules[modnum];

    VecPush(project->build_list, modnum);

    for (i = 0; i < VecCount(module->imports); i++) {
      u32 import = module->imports[i].module;
      if (!HashMapContains(&scan_set, import)) {
        HashMapSet(&scan_set, import, 1);
        VecPush(scan_list, import);
      }
    }
  }

  DestroyHashMap(&scan_set);
  FreeVec(scan_list);

  return Ok(project);
}

Env *PrimitiveEnv(void)
{
  PrimDef *primitives = Primitives();
  u32 i;
  Env *env = ExtendEnv(NumPrimitives(), 0);
  for (i = 0; i < NumPrimitives(); i++) {
    EnvSet(Symbol(primitives[i].name), i, env);
  }
  return env;
}

u8 *LinkModules(Project *project)
{
  u32 i;
  u32 size = 0;
  u8 *data, *cur;

  Chunk *intro = CompileIntro(project->modules);
  Chunk *outro = CompileCallMod(VecCount(project->modules)-1);
  size = ChunkSize(intro) + ChunkSize(outro);
  for (i = 0; i < VecCount(project->modules); i++) {
    size += ChunkSize(project->modules[i].code);
  }
  data = NewVec(u8, size);
  RawVecCount(data) = size;
  cur = data;
  cur = SerializeChunk(intro, cur);
  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *mod = &project->modules[project->build_list[i]];
    cur = SerializeChunk(mod->code, cur);
  }
  SerializeChunk(outro, cur);

  return data;
}

Result BuildProject(Project *project)
{
  Result result;
  u32 i;
  Env *env = PrimitiveEnv();
  Program *program = NewProgram();
  char *symbols;
  u32 sym_size;

  DestroySymbols();

  result = ScanProjectDeps(project);
  if (IsError(result)) return result;

  for (i = 0; i < VecCount(project->build_list); i++) {
    Module *module = &project->modules[project->build_list[i]];

    result = ParseModule(module);
    if (IsError(result)) return result;

    PrintNode(module->ast);

    result = CompileModule(module, i, project->modules, env);
    if (IsError(result)) return result;
  }

  program->code = LinkModules(project);
  sym_size = ExportSymbols(&symbols);
  program->symbols = NewVec(char, sym_size);
  Copy(symbols, program->symbols, sym_size);
  RawVecCount(program->symbols) = sym_size;
  free(symbols);

  return Ok(program);
}
