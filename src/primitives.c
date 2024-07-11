#include "primitives.h"
#include "env.h"
#include "types.h"
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
  char *type;
} PrimDef;

static PrimDef primitives[] = {
  {"print", VMPrint, "fn(a, sym)"},
  {"debug", VMDebug, "fn(sym)"},
  {"==", 0, "fn(a, a, int)"},
  {"|", 0, "fn(a, b, pair(a, b))"},
  {"<>", 0, "fn(a, a, a)"},
  {"<", 0, "fn(a, a, int)"},
  {">", 0, "fn(a, a, int)"},
  {"<<", 0, "fn(int, int, int)"},
  {"+", 0, "fn(int, int, int)"},
  {"-", 0, "fn(int, int, int)"},
  {"^", 0, "fn(int, int, int)"},
  {"*", 0, "fn(int, int, int)"},
  {"/", 0, "fn(int, int, int)"},
  {"%", 0, "fn(int, int, int)"},
  {"&", 0, "fn(int, int, int)"},
  {"not", 0, "fn(a, int)"},
  {"-", 0, "fn(int, int)"},
  {"~", 0, "fn(int, int)"},
  {"#", 0, "fn(a, int)"},
  {"[]", 0, "fn(a, int, b)"},
  {"[:]", 0, "fn(a, int, int, b)"},
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

val PrimitiveTypes(void)
{
  u32 i;
  val context = 0;
  for (i = 0; i < ArrayCount(primitives); i++) {
    char *spec = primitives[i].type;
    val name = SymVal(Symbol(primitives[i].name));
    val type = ParseType(spec);
    assert(type);

    context = Pair(Pair(name, Generalize(type, 0)), context);
  }
  return context;
}
