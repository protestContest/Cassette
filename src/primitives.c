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

  if (a == 0 || IsInt(a) || IsSym(a)) {
    str = ValStr(a, 0);
    printf("%s\n", str);
    free(str);
  } else if (IsPair(a)) {
    printf("[");
    while (a && IsPair(a)) {
      str = ValStr(Head(a), 0);
      printf("%s", str);
      free(str);
      a = Tail(a);
      if (a && IsPair(a)) {
        printf(", ");
      }
    }
    if (a) {
      printf(" | ");
      str = ValStr(a, 0);
      printf("%s", str);
      free(str);
    }
    printf("]\n");
  }

  StackPush(SymVal(Symbol("ok")), vm);
  return vmOk;
}

VMStatus VMDebug(VM *vm)
{
  getchar();
  StackPush(0, vm);
  return vmOk;
}

typedef struct {
  char *name;
  PrimFn fn;
} PrimDef;

static PrimDef primitives[] = {
  {"print", VMPrint},
  {"debug", VMDebug}
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
