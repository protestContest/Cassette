#include "primitives.h"
#include "env.h"
#include "types.h"
#include "vm.h"
#include <univ/vec.h>
#include <univ/symbol.h>

VMStatus VMPrint(VM *vm)
{
  val a, str;
  CheckStack(vm, 1);
  a = StackPop(vm);

  str = InspectVal(a);
  printf("%*.*s\n", BinaryLength(str), BinaryLength(str), BinaryData(str));

  StackPush(SymVal(Symbol("ok")), vm);
  return vmOk;
}

VMStatus VMFormat(VM *vm)
{
  val a;
  CheckStack(vm, 1);
  a = StackPop(vm);
  StackPush(FormatVal(a), vm);
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
