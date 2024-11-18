#include "project.h"
#include "compile.h"
#include "mem.h"
#include "ops.h"
#include "parse.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include "univ/vec.h"
#include "vm.h"

static Error **FileNotFound(char **filename)
{
  Error **error;
  HLock(filename);
  error = NewError("File \"^\" not found", *filename, 0, 0);
  (*error)->message = FormatString((*error)->message, *filename);
  HUnlock(filename);
  return error;
}

static Error **ModuleNotFound(char **name, char **file, u32 pos, u32 len)
{
  Error **error;
  HLock(file);
  error = NewError("Module \"^\" not found", *file, pos, len);
  HUnlock(file);
  HLock(name);
  (*error)->message = FormatString((*error)->message, *name);
  HUnlock(name);
  return error;
}

static Error **CircularDependency(char **mod1, char **mod2, char **file)
{
  Error **error;
  HLock(file);
  error = NewError("Circular dependency between ^ and ^", *file, 0, 0);
  HUnlock(file);
  HLock(mod1);
  (*error)->message = FormatString((*error)->message, *mod1);
  HUnlock(mod1);
  HLock(mod2);
  (*error)->message = FormatString((*error)->message, *mod2);
  HUnlock(mod2);
  return error;
}

Project **NewProject(void)
{
  Project **project = New(Project);
  InitVec((*project)->modules);
  InitHashMap(&(*project)->mod_map);
  (*project)->program = 0;
  InitVec((*project)->build_list);
  return project;
}

void FreeProject(Project **project)
{
  u32 i;
  for (i = 0; i < VecCount((*project)->modules); i++) {
    DestroyModule(&VecAt((*project)->modules, i));
  }
  FreeVec((*project)->modules);
  DestroyHashMap(&(*project)->mod_map);
  FreeVec((*project)->build_list);
  DisposeHandle((Handle)project);
}

Error **AddProjectFile(Project **project, char **filename)
{
  u32 i;
  Module mod;
  char **source;
  for (i = 0; i < VecCount((*project)->modules); i++) {
    Module *mod = &VecAt((*project)->modules, i);
    if (StrEq(*mod->filename, *filename)) return 0;
  }
  source = ReadFile(*filename);
  if (!source) return FileNotFound(filename);
  InitModule(&mod);
  mod.filename = filename;
  HandToHand(&mod.filename);
  mod.source = source;
  VecPush((*project)->modules, mod);
  return 0;
}

void ScanProjectFolder(Project **project, char *path)
{
  u32 i;
  FileList list = ListFiles(path, ".ct", 0);

  for (i = 0; i < VecCount(list); i++) {
    Error **error = AddProjectFile(project, VecAt(list, i));
    if (error) FreeError(error);
  }
  FreeFileList(list);
}

static Error **ScanModuleDeps(
  u32 mod_index,
  WordVec(scan_list),
  HashMap *build_set,
  HashMap *scan_set,
  Project **project)
{
  u32 i;
  Error **error;
  Module *mod = &VecAt((*project)->modules, mod_index);
  ASTNode **imports = ModuleImports(mod);

  mod->id = VecCount((*project)->build_list) + VecCount(scan_list);

  if (NodeCount(imports) > 0) {
    VecPush(scan_list, mod_index);
    HashMapSet(scan_set, mod_index, 1);
    for (i = 0; i < NodeCount(imports); i++) {
      ASTNode **import = NodeChild(imports, i);
      u32 name = NodeValue(NodeChild(import, 0));
      u32 index;
      if (!HashMapContains(&(*project)->mod_map, name)) {
        char **name_str = SymbolName(name);
        Error **error = ModuleNotFound(name_str, mod->filename, (*import)->start, (*import)->end - (*import)->start);
        DisposeHandle(name_str);
        return error;
      }

      index = HashMapGet(&(*project)->mod_map, name);
      if (HashMapContains(build_set, index)) continue;
      if (HashMapContains(scan_set, index)) {
        char **mod_name = SymbolName(NodeValue(ModuleName(mod)));
        char **name_str = SymbolName(name);
        Error **error = CircularDependency(mod_name, name_str, mod->filename);
        DisposeHandle(mod_name);
        DisposeHandle(name_str);
        return error;
      }

      error = ScanModuleDeps(index, scan_list, build_set, scan_set, project);
      if (error) return error;
    }
    VecPop(scan_list);
    HashMapDelete(scan_set, mod_index);
  }

  VecPush((*project)->build_list, mod_index);
  HashMapSet(build_set, mod_index, 1);
  return 0;
}

static Error **ScanDeps(Project **project)
{
  WordVec scan_list;
  HashMap build_set = EmptyHashMap;
  HashMap scan_set = EmptyHashMap;
  Error **error;
  InitVec(scan_list);
  error = ScanModuleDeps(0, scan_list, &build_set, &scan_set, project);
  FreeVec(scan_list);
  DestroyHashMap(&build_set);
  DestroyHashMap(&scan_set);
  return error;
}

static void LinkModules(Project **project)
{
  u32 i;
  Chunk **intro_chunk = 0;
  u32 size = 0;
  Program **program = NewProgram();
  HashMap strings = EmptyHashMap;
  u8 *cur;

  if (VecCount((*project)->build_list) > 1) {
    intro_chunk = NewChunk(0);
    ChunkWrite(opTuple, intro_chunk);
    ChunkWriteInt(VecCount((*project)->build_list) - 1, intro_chunk);
    ChunkWrite(opPull, intro_chunk);
    ChunkWriteInt(regMod, intro_chunk);
    size += ChunkSize(intro_chunk);
  }

  for (i = 0; i < VecCount((*project)->build_list); i++) {
    Module *mod = &VecAt((*project)->modules, VecAt((*project)->build_list, i));
    size += ChunkSize(mod->code);
  }

  (*program)->code = NewVec(u8, size);
  cur = VecData((*program)->code);
  if (intro_chunk) {
    AddChunkSource(intro_chunk, 0, &(*program)->srcmap);
    cur = SerializeChunk(intro_chunk, cur);
    FreeChunk(intro_chunk);
  }

  for (i = 0; i < VecCount((*project)->build_list); i++) {
    Module *mod = &VecAt((*project)->modules, VecAt((*project)->build_list, i));
    AddChunkSource(mod->code, mod->filename, &(*program)->srcmap);
    AddStrings(mod->ast, program, &strings);
    cur = SerializeChunk(mod->code, cur);
  }

  DestroyHashMap(&strings);
  VecCount((*program)->code) = size;
  (*project)->program = program;
}

Error **BuildProject(Project **project)
{
  u32 i;
  Error **error;
  Compiler c;

  SetSymbolSize(valBits);

  for (i = 0; i < VecCount((*project)->modules); i++) {
    Module *mod = &VecAt((*project)->modules, i);
    u32 name, j;
    ASTNode **exports;
    mod->ast = ParseModule(mod->source);
    if (IsErrorNode(mod->ast)) {
      char **msg = SymbolName((*mod->ast)->data.value);
      u32 len = (*mod->ast)->end - (*mod->ast)->start;
      Error **error;
      HLock(msg);
      error = NewError(*msg, *mod->filename, (*mod->ast)->start, len);
      HUnlock(msg);
      DisposeHandle(msg);
      return error;
    }

    name = NodeValue(ModuleName(mod));
    if (name) HashMapSet(&(*project)->mod_map, name, i);
    exports = ModuleExports(mod);
    for (j = 0; j < NodeCount(exports); j++) {
      u32 export = NodeValue(NodeChild(exports, j));
      HashMapSet(&mod->exports, export, j);
    }
  }

  error = ScanDeps(project);
  if (error) return error;

  InitCompiler(&c, (*project)->modules, &(*project)->mod_map);
  for (i = 0; i < VecCount((*project)->build_list); i++) {
    Module *mod = &VecAt((*project)->modules, VecAt((*project)->build_list, i));
    c.mod_id = mod->id;
    mod->code = Compile(mod->ast, &c);
    if (c.error) {
      (*c.error)->filename = mod->filename;
      HandToHand((Handle*)&(*c.error)->filename);
      return c.error;
    }
  }
  DestroyCompiler(&c);

  LinkModules(project);

  return 0;
}
