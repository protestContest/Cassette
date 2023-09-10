#include "function.h"
#include "lib.h"

static PrimitiveDef *primitives = NULL;

bool IsFunc(Val func, Heap *mem)
{
  return IsTaggedWith(func, Function, mem);
}

Val MakeFunc(Val entry, Val env, Heap *mem)
{
  Val func = MakeTuple(3, mem);
  TupleSet(func, 0, Function, mem);
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

void SeedPrimitives(void)
{
  if (primitives == NULL) {
    PrimitiveDef *stdlib = StdLib();
    for (u32 i = 0; i < StdLibSize(); i++) {
      VecPush(primitives, stdlib[i]);
    }
  }
}

void AddPrimitive(char *module, char *name, PrimitiveImpl fn)
{
  SeedPrimitives();
  PrimitiveDef def = {module, name, fn};
  VecPush(primitives, def);
}

bool IsPrimitive(Val value, Heap *mem)
{
  return IsTaggedWith(value, Primitive, mem);
}

Val DoPrimitive(Val prim, VM *vm)
{
  u32 index = RawInt(Tail(prim, &vm->mem));
  return primitives[index].impl(vm, StackPop(vm));
}

Val PrimitiveMap(Heap *mem)
{
  Assert(Eq(Primitive, MakeSymbol("*primitive*", mem)));

  Val prims = MakeMap(mem);
  for (u32 i = 0; i < VecCount(primitives); i++) {
    Val primitive = Pair(Primitive, IntVal(i), mem);

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
