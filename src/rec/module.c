#include "module.h"
#include "debug.h"
#include "parse.h"
#include "compile.h"

ModuleResult LoadModule(char *path, Heap *mem)
{
  char *source = ReadFile(path);

  Val ast = Parse(source, mem);

  if (IsTagged(ast, "error", mem)) {
    Inspect(ListAt(ast, 2, mem), mem);
    Print(":");
    Inspect(ListAt(ast, 3, mem), mem);
    Print("\n");
    return (ModuleResult){false, {.error = {nil, "Parse error"}}};
  }

  Print("Compiling ");
  Print(path);
  Print("\n");

  return Compile(ast, nil, mem);
}

ModuleResult LoadProject(char *source_folder, char *entry_file, Heap *mem)
{
  entry_file = JoinPath(source_folder, entry_file);
  char **files = ListFiles(source_folder);

  i32 entry = -1;
  Module *modules = NULL;
  HashMap mod_map = EmptyHashMap;

  for (i32 i = 0; i < (i32)VecCount(files); i++) {
    if (StrEq(files[i], entry_file)) {
      entry = VecCount(modules);
    } else {
      u32 len = StrLen(files[i]);
      if (len < 4 || !StrEq(files[i] + len - 4, ".cst")) {
        continue;
      }
    }

    ModuleResult result = LoadModule(files[i], mem);
    if (!result.ok) continue;

    if (IsNil(result.module.name) && !StrEq(files[i], entry_file)) continue;

    u32 key = result.module.name.as_i;
    if (HashMapContains(&mod_map, key)) {
      // merge modules of the same name

      Module old_mod = modules[HashMapGet(&mod_map, key)];
      for (u32 i = 0; i < result.module.imports.count; i++) {
        u32 key = HashMapKey(&result.module.imports, i);
        if (!HashMapContains(&old_mod.imports, key)) {
          HashMapSet(&old_mod.imports, key, 1);
        }
      }
      old_mod.code = AppendSeq(old_mod.code, result.module.code, mem);
    } else {
      HashMapSet(&mod_map, key, VecCount(modules));
      VecPush(modules, result.module);
    }
  }
  FreeVec(files);

  if (entry == -1) {
    Val name = BinaryFrom(entry_file, StrLen(entry_file), mem);
    return (ModuleResult){false, {.error = {name, "Could not find entry"}}};
  }

  ModuleResult result = (ModuleResult){true, {{nil, modules[entry].code, EmptyHashMap}}};
  u32 *needed = NULL;

  for (u32 i = 0; i < modules[entry].imports.count; i++) {
    VecPush(needed, HashMapKey(&modules[entry].imports, i));
  }

  HashMap loaded = EmptyHashMap;
  HashMapSet(&loaded, modules[entry].name.as_i, 1);
  while (VecCount(needed) > 0) {
    u32 key = VecPop(needed);
    if (!HashMapContains(&loaded, key)) {
      Module mod = modules[HashMapGet(&mod_map, key)];

      result.module.code = AppendSeq(mod.code, result.module.code, mem);
      for (u32 i = 0; i < mod.imports.count; i++) {
        VecPush(needed, HashMapKey(&mod.imports, i));
      }
      HashMapSet(&loaded, key, 1);
    }
  }

  FreeVec(needed);
  DestroyHashMap(&loaded);

  return result;
}
