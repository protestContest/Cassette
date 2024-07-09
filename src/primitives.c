#include "primitives.h"
#include "env.h"
#include <univ/vec.h>
#include <univ/symbol.h>

VMStatus VMPrint(VM *vm)
{
  val a;
  char *str;
  CheckStack(vm, 1);
  a = StackPop(vm);
  str = ValStr(a, 0);
  printf("%s\n", str);
  free(str);
  StackPush(SymVal(Symbol("ok")), vm);
  return vmOk;
}

typedef struct {
  char *name;
  PrimFn fn;
} PrimDef;

PrimDef primitives[] = {
  {"print", VMPrint}
};

void DefinePrimitives(VM *vm)
{
  u32 i;
  for (i = 0; i < ArrayCount(primitives); i++) {
    DefinePrimitive(Symbol(primitives[i].name), primitives[i].fn, vm);
  }
}

val PrimitiveEnv(void)
{
  u32 i;
  val frame = Tuple(ArrayCount(primitives));
  for (i = 0; i < ArrayCount(primitives); i++) {
    TupleSet(frame, i, SymVal(Symbol(primitives[i].name)));
  }
  return Pair(frame, 0);
}
