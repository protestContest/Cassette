#include "primitives.h"
#include <univ/symbol.h>
#include <univ/vec.h>

VMStatus VMPrint(VM *vm)
{
  val a, str;
  CheckStack(vm, 1);
  a = VMStackPop(vm);

  str = InspectVal(a);
  printf("%*.*s\n", BinaryLength(str), BinaryLength(str), BinaryData(str));

  VMStackPush(SymVal(Symbol("ok")), vm);
  return vmOk;
}

VMStatus VMFormat(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = VMStackPop(vm);
  VMStackPush(FormatVal(a), vm);
  return vmOk;
}

static PrimDef primitives[] = {
  {"print", VMPrint, "fn(a, sym)"},
  {"format", VMFormat, "fn(a, str)"}
};

PrimDef *Primitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}
