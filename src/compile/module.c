#include "module.h"
#include "parse.h"

Val MakeModule(Val name, Val body, Val imports, Val exports, Val filename, Mem *mem)
{
  Val module = MakeTuple(5, mem);
  TupleSet(module, 0, name, mem);
  TupleSet(module, 1, body, mem);
  TupleSet(module, 2, imports, mem);
  TupleSet(module, 3, exports, mem);
  TupleSet(module, 4, filename, mem);
  return module;
}

u32 CountExports(Val nodes, HashMap *modules, Mem *mem)
{
  u32 count = 0;
  while (nodes != Nil) {
    Val mod_name = NodeExpr(Head(nodes, mem), mem);
    if (HashMapContains(modules, mod_name)) {
      Val module = HashMapGet(modules, mod_name);
      count += ListLength(ModuleExports(module, mem), mem);
    }
    nodes = Tail(nodes, mem);
  }
  return count;
}
