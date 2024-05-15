#include "primitives.h"
#include "env.h"

VMStatus VMHead(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = StackPop(vm);
  if (!IsPair(a)) return invalidType;
  StackPush(Head(a), vm);
  return ok;
}

VMStatus VMTail(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = StackPop(vm);
  if (!IsPair(a)) return invalidType;
  StackPush(Tail(a), vm);
  return ok;
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
  StackPush(0, vm);
  return ok;
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
