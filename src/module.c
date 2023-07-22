#include "module.h"
#include "parse.h"
#include <unistd.h>
#include <dirent.h>

Val SplitModName(Val mod_name, Mem *mem)
{
  char *name = SymbolName(mod_name, mem);
  char *end = name;

  Val parts = nil;
  while (*end != '\0') {
    while (*end != '\0' && *end != '.') end++;

    Val part = MakeSymbolFrom(name, end-name, mem);
    parts = Pair(part, parts, mem);

    if (*end == '.') end++;
    name = end;
  }

  return ReverseList(parts, mem);
}

Val FindImports(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  if (!IsPair(ast)) return nil;

  if (IsTagged(ast, "import", mem)) {
    Val name = ListAt(ast, 1, mem);
    return Pair(name, nil, mem);
  }

  Val imports = nil;
  while (IsPair(ast) && !IsNil(ast)) {
    imports = ListConcat(imports, FindImports(Head(ast, mem), mem), mem);
    ast = Tail(ast, mem);
  }
  return imports;
}

Val LoadError(Val name, Mem *mem)
{
  return
    Pair(MakeSymbol("error", mem),
    Pair(MakeSymbol("load", mem),
    Pair(name, nil, mem), mem), mem);
}

void FindModules(char *path, HashMap *modules, Mem *mem)
{
  DIR *dir = opendir(path);
  if (dir == NULL) return;

  for (struct dirent *item = readdir(dir); item != NULL; item = readdir(dir)) {
    if (item->d_name[0] == '.') continue;

    if (item->d_type == DT_DIR) {
      FindModules(item->d_name, modules, mem);
    } else if (item->d_type == DT_REG) {
      char *item_path = JoinPath(path, item->d_name);
      char *source = ReadFile(item_path);
      if (source == NULL) continue;

      Val ast = Parse(source, mem);
      Val mod_name = ModuleName(ast, mem);
      if (!HashMapContains(modules, mod_name.as_i)) {
        Val name = ListAt(ast, 1, mem);
        HashMapSet(modules, name.as_i, ast.as_i);
      }
    }
  }
}

static Val WrapEntryModule(Val module, Mem *mem)
{
  Val mod_name = ModuleName(module, mem);
  Val run_mod =
    Pair(MakeSymbol("import", mem),
    Pair(mod_name,
    Pair(nil, nil, mem), mem), mem);
  return
    Pair(module,
    Pair(run_mod, nil, mem), mem);
}

Val LoadModule(char *entry, char *module_path, Mem *mem)
{
  char *source = (char*)ReadFile(entry);
  if (source == NULL) return LoadError(MakeSymbol(entry, mem), mem);

  Val ast = Parse(source, mem);
  if (IsTagged(ast, "error", mem)) return ast;

  Val entry_mod = ModuleName(ast, mem);
  ast = WrapEntryModule(ast, mem);

  Val imports = FindImports(ast, mem);
  if (IsNil(imports)) return ast;

  HashMap modules;
  InitHashMap(&modules);
  // don't try to load non-modules
  HashMapSet(&modules, SymbolFor("*").as_i, SymbolFor("*loaded*").as_i);
  // don't try to load the entry module
  HashMapSet(&modules, entry_mod.as_i, SymbolFor("*loaded*").as_i);

  FindModules(module_path, &modules, mem);

  while (!IsNil(imports)) {
    Val import = Head(imports, mem);
    if (!HashMapContains(&modules, import.as_i)) return LoadError(import, mem);

    Val module = (Val){.as_i = HashMapGet(&modules, import.as_i)};
    if (!Eq(module, SymbolFor("*loaded*"))) {
      HashMapSet(&modules, import.as_i, SymbolFor("*loaded*").as_i);
      ast = Pair(module, ast, mem);
      Val mod_imports = FindImports(module, mem);
      imports = ListConcat(imports, mod_imports, mem);
    }

    imports = Tail(imports, mem);
  }

  DestroyHashMap(&modules);

  return Pair(MakeSymbol("do", mem), ast, mem);
}
