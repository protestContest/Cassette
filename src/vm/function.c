#include "function.h"
#include "lib.h"

static PrimitiveDef primitives[] = {
  {NULL, "head", StdHead},
  {NULL, "tail", StdTail},
  {"IO", "print", IOPrint},
  {"IO", "inspect", IOInspect},
};

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
  return primitives[index].impl(vm, StackPop(vm));
}

Val DefinePrimitives(Heap *mem)
{
  Val prims = MakeMap(mem);
  for (u32 i = 0; i < ArrayCount(primitives); i++) {
    Val primitive = Pair(MakeSymbol("*prim*", mem), IntVal(i), mem);

    if (primitives[i].module == NULL) {
      prims = MapPut(prims, MakeSymbol(primitives[i].name, mem), primitive, mem);
    } else {
      Val mod_name = MakeSymbol(primitives[i].module, mem);
      if (MapContains(prims, mod_name, mem)) {
        Val mod = MapGet(prims, mod_name, mem);
        mod = MapPut(mod, MakeSymbol(primitives[i].name, mem), primitive, mem);
        prims = MapPut(prims, mod_name, mod, mem);
      } else {
        Val mod = MakeMap(mem);
        mod = MapPut(mod, MakeSymbol(primitives[i].name, mem), primitive, mem);
        prims = MapPut(prims, mod_name, mod, mem);
      }
    }
  }

  return prims;
}
