#include "function.h"
#include "primitives.h"

bool IsPrimitive(Val func, Mem *mem)
{
  return IsTagged(func, "*prim*", mem);
}

Val MakePrimitive(Val num, Mem *mem)
{
  return Pair(MakeSymbol("*prim*", mem), num, mem);
}

u32 PrimitiveNum(Val primitive, Mem *mem)
{
  return RawInt(Tail(primitive, mem));
}

bool IsFunction(Val func, Mem *mem)
{
  return IsTuple(func, mem) && Eq(TupleGet(func, 0, mem), SymbolFor("*func*"));
}

Val MakeFunction(Val pos, Val arity, Val env, Mem *mem)
{
  Val func = MakeTuple(4, mem);
  TupleSet(func, 0, MakeSymbol("*func*", mem), mem);
  TupleSet(func, 1, pos, mem);
  TupleSet(func, 2, env, mem);
  TupleSet(func, 3, arity, mem);
  return func;
}

u32 FunctionEntry(Val func, Mem *mem)
{
  return RawInt(TupleGet(func, 1, mem));
}

u32 FunctionArity(Val func, Mem *mem)
{
  return RawInt(TupleGet(func, 3, mem));
}

Val FunctionEnv(Val func, Mem *mem)
{
  return TupleGet(func, 2, mem);
}

Val ApplyPrimitive(Val func, u32 num_args, VM *vm)
{
  u32 index = PrimitiveNum(func, &vm->mem);
  return GetPrimitive(index)(num_args, vm);
}
