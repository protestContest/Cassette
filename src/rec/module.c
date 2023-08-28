#include "module.h"
#include "debug.h"
#include "parse.h"
#include "compile.h"

ModuleResult LoadModule(char *file, Heap *mem, Args *args)
{
  char *source = ReadFile(file);

  ParseResult ast = Parse(source, mem);

  if (!ast.ok) {
    return (ModuleResult){false, {.error = ast.error}};
  }

  if (args->verbose) {
    Print("Compiling ");
    Print(file);
    Print("\n");
  }

  return Compile(ast.value, nil, mem);
}

CompileResult LoadModules(Args *args, Heap *mem)
{
  // get all files from source directory
  char **files = ListFiles(args->dir);

  i32 entry = -1;
  Module *modules = NULL;
  HashMap mod_map = EmptyHashMap;

  // try to compile each file into a module
  for (i32 i = 0; i < (i32)VecCount(files); i++) {
    // only compile the entry file or files ending with ".cst"
    if (StrEq(files[i], args->entry)) {
      entry = VecCount(modules);
    } else {
      u32 len = StrLen(files[i]);
      if (len < 4 || !StrEq(files[i] + len - 4, ".cst")) {
        continue;
      }
    }

    ModuleResult result = LoadModule(files[i], mem, args);
    if (!result.ok) {
      PrintCompileError(&result.error, files[i]);
      // ignore modules that fail to compile (except the entry file)
      if (StrEq(files[i], args->entry)) {
        return (CompileResult){false, {.error = result.error}};
      } else {
        continue;
      }
    }
    result.module.file = files[i];

    // ignore files that aren't modules (except the entry file)
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
    return (CompileResult){false, {.error = {message, nil, 0}}};
  }

  // starting with the entry file, recursively prepend imported module code into
  // one big code sequence, including module identifiers

  Seq code = AppendSeq(ModuleSeq(modules[entry].file, mem), modules[entry].code, mem);
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

      code =
        AppendSeq(ModuleSeq(mod.file, mem),
        AppendSeq(mod.code,
        code, mem), mem);

      for (u32 i = 0; i < mod.imports.count; i++) {
        VecPush(needed, HashMapKey(&mod.imports, i));
      }
      HashMapSet(&loaded, key, 1);
    }
  }

  FreeVec(needed);
  DestroyHashMap(&loaded);

  return (CompileResult){true, {code}};
}
