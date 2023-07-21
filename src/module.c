#include "module.h"
#include "parse.h"

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

static Val LoadError(Val name, Mem *mem)
{
  return
    Pair(MakeSymbol("error", mem),
    Pair(MakeSymbol("load", mem),
    Pair(name, nil, mem), mem), mem);
}

void FindModules(char *path, HashMap *modules, Mem *mem)
{
  Folder folder = OpenFolder(path);
  if (folder == NULL) return;

  FolderItem item;
  do {
    item = NextFolderItem(folder);

    if (item.type == TypeFolder) {
      FindModules(item.name, modules, mem);
    } else if (item.type == TypeFile) {
      char *item_path = JoinPath(path, item.name);
      char *source = ReadFile(item_path);
      if (source == NULL) continue;

      Val ast = Parse(source, mem);
      if (IsTagged(ast, "module", mem)) {
        Val name = ListAt(ast, 1, mem);
        HashMapSet(modules, name.as_i, ast.as_i);
      }
    }
  } while (item.type != TypeNone);
}

Val LoadModule(char *entry, char *module_path, Mem *mem)
{
  char *source = (char*)ReadFile(entry);
  if (source == NULL) return LoadError(MakeSymbol(entry, mem), mem);

  Val ast = Parse(source, mem);
  if (IsTagged(ast, "error", mem)) return ast;

  Val imports = FindImports(ast, mem);
  if (IsNil(imports)) return ast;

  HashMap modules;
  InitHashMap(&modules);
  FindModules(module_path, &modules, mem);

  ast = Pair(ast, nil, mem);

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
