#include "module.h"
#include "debug.h"
#include "parse.h"
#include "compile.h"
#include "seq.h"
#include "vm.h"
#include "univ/file.h"
#include "univ/string.h"
#include "univ/system.h"
#include "univ/memory.h"

ModuleResult LoadModule(char *file, Heap *mem, Val env, CassetteOpts *opts)
{
  char *source = ReadFile(file);

  ParseResult ast = Parse(source, mem);

  if (!ast.ok) {
    return (ModuleResult){false, {.error = ast.error}};
  }

  return Compile(ast.value, opts, env, mem);
}

CompileResult LoadModules(CassetteOpts *opts, Heap *mem)
{
  // get all files from source directory
  FileSet fileset = ListFiles(opts->dir);

  i32 entry = -1;
  u32 num_modules = 0;
  Module modules[ModuleMax];
  HashMap mod_map = EmptyHashMap;
  Val env = CompileEnv(mem);

  // try to compile each file into a module
  for (u32 i = 0; i < fileset.count; i++) {
    // only compile the entry file or files ending with ".cst"
    if (StrEq(fileset.files[i], opts->entry)) {
      entry = num_modules;
    } else {
      u32 len = StrLen(fileset.files[i]);
      if (len < 4 || !StrEq(fileset.files[i] + len - 4, ".cst")) {
        continue;
      }
    }

    ModuleResult result = LoadModule(fileset.files[i], mem, env, opts);
    if (!result.ok) {
      result.error.file = fileset.files[i];
      return (CompileResult){false, {.error = result.error}};
    }
    result.module.file = fileset.files[i];

    // ignore files that aren't modules (except the entry file)
    if (IsNil(result.module.name) && !StrEq(fileset.files[i], opts->entry)) continue;

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
      Assert(num_modules < ModuleMax);
      HashMapSet(&mod_map, key, num_modules);
      modules[num_modules++] = result.module;
    }
  }
  Free(fileset.files);

  if (entry == -1) {
    return (CompileResult){false, {.error = {"Could not find entry", NULL, 0}}};
  }

  // starting with the entry file, recursively prepend imported module code into
  // one big code sequence, including module identifiers

  Seq code = AppendSeq(ModuleSeq(modules[entry].file, mem), modules[entry].code, mem);
  u32 num_needed = 0;
  u32 needed[ModuleMax];
  for (u32 i = 0; i < modules[entry].imports.count; i++) {
    needed[num_needed++] = HashMapKey(&modules[entry].imports, i);
  }
  HashMap loaded = EmptyHashMap;
  HashMapSet(&loaded, modules[entry].name.as_i, 1);

  while (num_needed > 0) {
    u32 key = needed[--num_needed];
    if (!HashMapContains(&loaded, key)) {
      if (!HashMapContains(&mod_map, key)) {
        CompileResult result = {false, {.error = {"", NULL, 0}}};
        result.error.message = "Could not find module";
        return result;
      }
      Module mod = modules[HashMapGet(&mod_map, key)];

      code =
        AppendSeq(ModuleSeq(mod.file, mem),
        AppendSeq(mod.code,
        code, mem), mem);

      for (u32 i = 0; i < mod.imports.count; i++) {
        needed[num_needed++] = HashMapKey(&mod.imports, i);
      }
      HashMapSet(&loaded, key, 1);
    }
  }

  if (!IsNil(modules[entry].name)) {
    Val after_load = MakeLabel();
    code = AppendSeq(code,
      MakeSeq(0, 0,
        Pair(IntVal(OpLoad),
        Pair(ModuleRef(modules[entry].name, mem),
        Pair(IntVal(OpCont),
        Pair(LabelRef(after_load, mem),
        Pair(IntVal(OpApply),
        Pair(Label(after_load, mem), nil, mem), mem), mem), mem), mem), mem)), mem);
  }

  DestroyHashMap(&loaded);

  return (CompileResult){true, {code}};
}
