#include "module.h"
#include "parse.h"

typedef struct {
  Val ast;
  Val imports;
  Mem *mem;
} Module;

Val FindImports(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  if (!IsPair(ast)) return nil;

  if (IsTagged(ast, "import", mem)) {
    return Pair(ListAt(ast, 1, mem), nil, mem);
  }

  Val imports = nil;
  while (IsPair(ast) && !IsNil(ast)) {
    imports = ListConcat(imports, FindImports(Head(ast, mem), mem), mem);
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

Val WrapModule(Val ast, Val name, Mem *mem)
{
  return
    Pair(MakeSymbol("defmod", mem),
    Pair(name,
    Pair(
      Pair(MakeSymbol("->", mem),
      Pair(nil,
      Pair(ast, nil, mem), mem), mem), nil, mem), mem), mem);
}

Val LoadModule(char *entry, Mem *mem)
{
  Map map;
  InitMap(&map);

  char *source = (char*)ReadFile(entry);
  if (source == NULL) return Pair(SymbolFor("error"), MakeSymbol("not_found", mem), mem);

  Val ast = Parse(source, mem);

  if (IsTagged(ast, "do", mem)) {
    ast = Tail(ast, mem);
  } else {
    ast = Pair(ast, nil, mem);
  }

  Val imports = FindImports(ast, mem);

  while (!IsNil(imports)) {
    Val import = Head(imports, mem);
    if (!MapContains(&map, import.as_i)) {
      source = (char*)ReadFile(SymbolName(import, mem));
      if (source == NULL) {
        Print("Could not load \"");
        Print(SymbolName(import, mem));
        Print("\"\n");
        return Pair(SymbolFor("error"), MakeSymbol("not_found", mem), mem);
      }

      Val mod = Parse(source, mem);
      ast = Pair(WrapModule(mod, import, mem), ast, mem);
      imports = ListConcat(imports, FindImports(mod, mem), mem);
      MapSet(&map, import.as_i, 1);
    }
    imports = Tail(imports, mem);
  }

  return Pair(MakeSymbol("do", mem), ast, mem);
}

void PrintModule(Module *mod)
{
  Print("Module ");
  Print("\n  ");
  PrintVal(mod->ast, mod->mem);
  Print("\n");
  Print("Imports: ");
  PrintVal(mod->imports, mod->mem);
  Print("\n");
  PrintMem(mod->mem);
}
