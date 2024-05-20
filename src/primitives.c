#include "primitives.h"
#include "env.h"

VMStatus VMHead(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = StackPop(vm);
  if (!IsPair(a)) return invalidType;
  StackPush(Head(a), vm);
  return vmOk;
}

VMStatus VMTail(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = StackPop(vm);
  if (!IsPair(a)) return invalidType;
  StackPush(Tail(a), vm);
  return vmOk;
}

VMStatus VMPrint(VM *vm)
{
  val a;
  char *str;
  CheckStack(vm, 1);
  a = StackPop(vm);
  str = ValStr(a);
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
  {"head", VMHead},
  {"tail", VMTail},
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
  return 0;
}
