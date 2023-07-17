#include "proc.h"
#include "primitives.h"

bool IsPrimitive(Val proc, Mem *mem)
{
  return IsPair(proc) && Eq(Head(proc, mem), SymbolFor("*prim*"));
}

Val MakePrimitive(Val num, Mem *mem)
{
  return Pair(MakeSymbol("*prim*", mem), num, mem);
}

u32 PrimitiveNum(Val primitive, Mem *mem)
{
  return RawInt(Tail(primitive, mem));
}

bool IsFunction(Val proc, Mem *mem)
{
  return IsTuple(proc, mem) && Eq(TupleGet(proc, 0, mem), SymbolFor("*func*"));
}

Val MakeFunction(Val pos, Val env, Mem *mem)
{
  Val func = MakeTuple(3, mem);
  TupleSet(func, 0, MakeSymbol("*func*", mem), mem);
  TupleSet(func, 1, pos, mem);
  TupleSet(func, 2, env, mem);
  return func;
}

u32 ProcEntry(Val proc, Mem *mem)
{
  return RawInt(TupleGet(proc, 1, mem));
}

Val ProcEnv(Val proc, Mem *mem)
{
  return TupleGet(proc, 2, mem);
}

Val ApplyPrimitive(Val proc, u32 num_args, VM *vm)
{
  u32 index = PrimitiveNum(proc, &vm->mem);
  return GetPrimitive(index)(num_args, vm);
}
