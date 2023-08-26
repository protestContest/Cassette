#include "module.h"
#include "debug.h"
#include "parse.h"
#include "compile.h"

ModuleResult LoadModule(char *source, Heap *mem, Args *args)
{
  ParseResult ast = Parse(source, mem);

  if (!ast.ok) {
    return (ModuleResult){false, {.error = ast.error}};
  }

  return Compile(ast.value, nil, mem);
}

ModuleResult LoadModules(Args *args, Heap *mem)
{
  char **files = ListFiles(args->dir);

  i32 entry = -1;
  Module *modules = NULL;
  HashMap mod_map = EmptyHashMap;

  for (i32 i = 0; i < (i32)VecCount(files); i++) {
    if (StrEq(files[i], args->entry)) {
      entry = VecCount(modules);
    } else {
      u32 len = StrLen(files[i]);
      if (len < 4 || !StrEq(files[i] + len - 4, ".cst")) {
        continue;
      }
    }

    char *source = ReadFile(files[i]);
    ModuleResult result = LoadModule(source, mem, args);
    if (!result.ok) {
      PrintCompileError(&result.error, source);
      if (StrEq(files[i], args->entry)) {
        return result;
      } else {
        continue;
      }
    }

    if (IsNil(result.module.name) && !StrEq(files[i], args->entry)) continue;

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
    char *message = StrCat("Could not find entry ", args->entry);
    return (ModuleResult){false, {.error = {message, nil, 0}}};
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
