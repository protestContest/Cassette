#include "module.h"

Val MakeModule(Val name, Val stmts, Val imports, Val exports, Val filename, Mem *mem)
{
  Val module = MakeTuple(5, mem);
  TupleSet(module, 0, name, mem);
  TupleSet(module, 1, stmts, mem);
  TupleSet(module, 2, imports, mem);
  TupleSet(module, 3, exports, mem);
  TupleSet(module, 4, filename, mem);
  return module;
}

Val ModuleName(Val mod, Mem *mem)
{
  return TupleGet(mod, 0, mem);
}

Val ModuleBody(Val mod, Mem *mem)
{
  return TupleGet(mod, 1, mem);
}

Val ModuleImports(Val mod, Mem *mem)
{
  return TupleGet(mod, 2, mem);
}

Val ModuleExports(Val mod, Mem *mem)
{
  return TupleGet(mod, 3, mem);
}

char *ModuleFile(Val mod, Mem *mem, SymbolTable *symbols)
{
  return SymbolName(TupleGet(mod, 4, mem), symbols);
}
