#include "primitives.h"
#include <univ/symbol.h>
#include <univ/vec.h>

Result VMPrint(VM *vm)
{
  val a, str;
  a = VMStackPop(vm);

  str = InspectVal(a);
  printf("%*.*s\n", BinaryLength(str), BinaryLength(str), BinaryData(str));

  return OkVal(SymVal(Symbol("ok")));
}

Result VMFormat(VM *vm)
{
  val a;
  a = VMStackPop(vm);
  VMStackPush(FormatVal(a), vm);
  return OkVal(FormatVal(a));
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
