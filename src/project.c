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

static Result ModuleNotFound(char *name, char *file, u32 pos)
{
  u32 len = strlen(name);
  Error *error = NewError(0, file, pos, len);
  error->message = StrCat(error->message, "Module \"");
  error->message = StrCat(error->message, name);
  error->message = StrCat(error->message, "\" not found");
  return Err(error);
}

static Result DuplicateModule(char *name, char *file)
{
  Error *error = NewError(0, file, 0, 0);
  error->message = StrCat(error->message, "Duplicate module \"");
  error->message = StrCat(error->message, name);
  error->message = StrCat(error->message, "\"");
  return Err(error);
}

static FileList *ListProjectFiles(char *entryfile, char *searchpath)
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

static u8 *LinkModules(Project *project)
{
  u32 i;
  u32 size = 0;
  u8 *data, *cur;
  u32 num_modules = VecCount(project->build_list);

  Chunk *intro = CompileIntro(num_modules);
  Chunk *outro = CompileCallMod(num_modules-1);
  ChunkWrite(opDrop, outro);
  ChunkWrite(opNoop, outro);
  size = ChunkSize(intro) + ChunkSize(outro);
  for (i = 0; i < VecCount(project->modules); i++) {
    size += ChunkSize(project->modules[i].code);
  }
  data = NewVec(u8, size);
  RawVecCount(data) = size;
  cur = data;
  cur = SerializeChunk(intro, cur);
  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 modnum = project->build_list[num_modules - 1 - i];
    Module *mod = &project->modules[project->build_list[modnum]];
    cur = SerializeChunk(mod->code, cur);
  }
  SerializeChunk(outro, cur);

  return data;
}

Result BuildProject(Project *project)
{
  Result result;
  u32 i;
  Env *env = 0;
  Program *program = NewProgram();
  char *symbols;
  u32 sym_size;

  DestroySymbols();
  SetSymbolSize(valBits);

  result = ScanProjectDeps(project);
  if (IsError(result)) return result;

  for (i = 0; i < VecCount(project->build_list); i++) {
    u32 modnum = project->build_list[i];
    Module *module = &project->modules[modnum];

    result = ParseModuleBody(module);
    if (IsError(result)) return result;

    result = CompileModule(module, project->modules, env);
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
