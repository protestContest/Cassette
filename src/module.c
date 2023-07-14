#include "module.h"
#include "parse.h"

Val FindImports(Val ast, char *folder, Mem *mem)
{
  if (IsNil(ast)) return nil;
  if (!IsPair(ast)) return nil;

  if (IsTagged(ast, "import", mem)) {
    Val rest = Tail(ast, mem);
    Val import = MakeSymbol(JoinPath(folder, SymbolName(Head(rest, mem), mem)), mem);
    SetHead(rest, import, mem);
    return Pair(import, nil, mem);
  }

  Val imports = nil;
  while (IsPair(ast) && !IsNil(ast)) {
    imports = ListConcat(imports, FindImports(Head(ast, mem), folder, mem), mem);
    ast = Tail(ast, mem);
  }
  return imports;
}

Val FindExports(Val ast, Mem *mem)
{
  if (!IsPair(ast)) return nil;

  if (IsTagged(ast, "mod", mem)) {
    Val mod = ListAt(ast, 2, mem);
    return FindExports(ListAt(mod, 2, mem), mem);
  }

  Val exports = nil;
  while (!IsNil(ast)) {
    Val node = Head(ast, mem);
    if (IsTagged(node, "let", mem)) {
      node = Tail(node, mem);
      while (!IsNil(node)) {
        Val assign = Head(node, mem);
        exports = Pair(Head(assign, mem), exports, mem);
        node = Tail(node, mem);
      }
    }
    ast = Tail(ast, mem);
  }

  return exports;
}

static Val WrapModule(Val ast, Val name, Mem *mem)
{
  return
    Pair(MakeSymbol("defmod", mem),
    Pair(name,
    Pair(
      Pair(MakeSymbol("->", mem),
      Pair(nil,
      Pair(ast, nil, mem), mem), mem), nil, mem), mem), mem);
}

static Val LoadError(char *filename, Mem *mem)
{
  return
    Pair(MakeSymbol("error", mem),
    Pair(MakeSymbol("load", mem),
    Pair(MakeSymbol(filename, mem), nil, mem), mem), mem);
}

Val LoadModule(char *entry, Mem *mem)
{
  Map map;
  InitMap(&map);

  char *source = (char*)ReadFile(entry);
  if (source == NULL) return LoadError(entry, mem);

  Val ast = Parse(source, mem);
  if (IsTagged(ast, "error", mem)) return ast;

  // we just want a list of statements, so we can prepend the modules
  if (IsTagged(ast, "do", mem)) {
    ast = Tail(ast, mem);
  } else {
    ast = Pair(ast, nil, mem);
  }

  char *folder = FolderName(entry);
  Val imports = FindImports(ast, folder, mem);

  while (!IsNil(imports)) {
    Val import = Head(imports, mem);

    if (!MapContains(&map, import.as_i)) {
      char *import_name = SymbolName(import, mem);
      char *folder = FolderName(import_name);

      source = (char*)ReadFile(import_name);
      if (source == NULL) return LoadError(SymbolName(import, mem), mem);

      Val mod = Parse(source, mem);
      if (IsTagged(mod, "error", mem)) return mod;

      ast = Pair(WrapModule(mod, import, mem), ast, mem);
      imports = ListConcat(imports, FindImports(mod, folder, mem), mem);
      MapSet(&map, import.as_i, 1);
    }
    imports = Tail(imports, mem);
  }

  return Pair(MakeSymbol("do", mem), ast, mem);
}
