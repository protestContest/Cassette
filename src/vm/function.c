#include "function.h"
#include "primitive.h"

static PrimitiveDef builtins[] = {
  {"head", PrimHead},
  {"tail", PrimTail},
};

static u32 prim_count = 0;
static PrimitiveDef *primitives = NULL;

bool IsFunc(Val func, Heap *mem)
{
  return IsTagged(func, "*func*", mem);
}

Val MakeFunc(Val entry, Val env, Heap *mem)
{
  Val func = MakeTuple(3, mem);
  TupleSet(func, 0, MakeSymbol("*func*", mem), mem);
  TupleSet(func, 1, entry, mem);
  TupleSet(func, 2, env, mem);
  return func;
}

Val FuncEntry(Val func, Heap *mem)
{
  return TupleGet(func, 1, mem);
}

Val FuncEnv(Val func, Heap *mem)
{
  return TupleGet(func, 2, mem);
}

bool IsPrimitive(Val value, Heap *mem)
{
  return IsTagged(value, "*prim*", mem);
}

Val DoPrimitive(Val prim, VM *vm)
{
  u32 index = RawInt(Tail(prim, vm->mem));
  if (index < ArrayCount(builtins)) {
    return builtins[index].impl(vm);
  } else {
    index -= ArrayCount(builtins);
    return primitives[index].impl(vm);
  }
}

void SetPrimitives(PrimitiveDef *prims, u32 count)
{
  primitives = prims;
  prim_count = count;
}

Val PrimitiveNames(Heap *mem)
{
  u32 num_builtins = ArrayCount(builtins);
  u32 num = num_builtins + prim_count;
  Val names = MakeTuple(num, mem);
  for (u32 i = 0; i < num_builtins; i++) {
    TupleSet(names, i, MakeSymbol(builtins[i].name, mem), mem);
  }
  for (u32 i = num_builtins; i < num; i++) {
    TupleSet(names, i, MakeSymbol(primitives[i-num_builtins].name, mem), mem);
  }
  return names;
}

Val GetPrimitives(Heap *mem)
{
  u32 num = ArrayCount(builtins) + prim_count;
  Val prims = MakeTuple(num, mem);
  for (u32 i = 0; i < num; i++) {
    Val p = Pair(MakeSymbol("*prim*", mem), IntVal(i), mem);
    TupleSet(prims, i, p, mem);
  }
  return prims;
}
