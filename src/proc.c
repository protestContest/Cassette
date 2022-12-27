#include "proc.h"
#include "env.h"
#include "list.h"

typedef Val (*PrimitiveImpl)(VM *vm, Val args);

typedef struct {
  char *sym;
  PrimitiveImpl impl;
} PrimitiveProc;

Val Add(VM *vm, Val args)
{
  float sum = 0;
  while (!IsNil(args)) {
    sum += RawVal(Head(vm, args));
    args = Tail(vm, args);
  }
  return NumVal(sum);
}

#define NUM_PRIMITIVES 1
static PrimitiveProc primitives[NUM_PRIMITIVES] = {
  {"+", Add}
};

Val ApplyPrimitiveProc(VM *vm, Val proc, Val args)
{
  for (u32 i = 0; i < NUM_PRIMITIVES; i++) {
    if (Eq(SymbolFor(vm, primitives[i].sym, strlen(primitives[i].sym)), Tail(vm, proc))) {
      return primitives[i].impl(vm, args);
    }
  }

  Error("Primitive procedure not implemented");
}

void DefinePrimitives(VM *vm, Val env)
{
  Val primitive_val = MakeSymbol(vm, "primitive", 9);

  for (u32 i = 0; i < NUM_PRIMITIVES; i++) {
    Val sym = MakeSymbol(vm, primitives[i].sym, strlen(primitives[i].sym));
    Val proc = MakePair(vm, primitive_val, sym);
    Define(vm, sym, proc, env);
  }
}

Val MakeProcedure(VM *vm, Val params, Val body, Val env)
{
  return MakeList(vm, 4, fn_val, params, body, env);
}
