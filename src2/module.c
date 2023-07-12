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
    return Pair(Tail(ast, mem), nil, mem);
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

static Module LoadModule(Val name, Mem *mem)
{
  Module mod;
  mod.mem = mem;

  char *filename = SymbolName(name, mem);

  char *source = (char*)ReadFile(filename);
  Val ast = Parse(source, mem);
  mod.ast = WrapModule(ast, name, mem);
  mod.imports = FindImports(mod.ast, mem);
  return mod;
}

Val LoadModules(char *entry, Mem *mem)
{
  Module *mods = NULL;
  Map map;
  InitMap(&map);

  Val entry_name = MakeSymbol(entry, mem);

  VecPush(mods, LoadModule(entry_name, mem));
  MapSet(&map, Hash(entry, StrLen(entry)), 1);

  for (u32 i = 0; i < VecCount(mods); i++) {
    Val imports = mods[i].imports;
    while (!IsNil(imports)) {
      Val name = Head(imports, mem);
      if (!MapContains(&map, name.as_i)) {
        VecPush(mods, LoadModule(name, mem));
        MapSet(&map, name.as_i, 1);
      }
      imports = Tail(imports, mem);
    }
  }

  Val ast = Pair(Pair(MakeSymbol("import", mem), entry_name, mem), nil, mem);
  for (u32 i = 0; i < VecCount(mods); i++) {
    ast = Pair(mods[i].ast, ast, mem);
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
