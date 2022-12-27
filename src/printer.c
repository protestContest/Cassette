#include "printer.h"
#include "list.h"

void PrintTail(VM *vm, Val tail)
{
  if (IsNil(tail)) {
    printf(")");
    return;
  }

  if (IsLoop(vm, tail)) {
    printf(" ^)");
    return;
  }

  printf(" ");
  PrintValue(vm, Head(vm, tail));
  PrintTail(vm, Tail(vm, tail));
}

void PrintValue(VM *vm, Val val)
{
  switch (TypeOf(val)) {
  case NUMBER:
    printf("%.1f", (float)RawVal(val));
    break;
  case INTEGER:
    printf("%d", (u32)RawVal(val));
    break;
  case PAIR:
    printf("(");
    if (IsNil(val)) {
      printf(")");
    } else {
      if (Eq(val, Head(vm, val))) {
        printf("_");
      } else {
        PrintValue(vm, Head(vm, val));
      }

      if (Eq(val, Tail(vm, val))) {
        printf(" . ^)");
      } else if (IsPair(Tail(vm, val))) {
        PrintTail(vm, Tail(vm, val));
      } else {
        printf(" . ");
        PrintValue(vm, Tail(vm, val));
        printf(")");
      }
    }
    break;
  case SYMBOL:
    for (u32 i = 0; i < BinaryLength(vm, SymbolName(vm, val)); i++) {
      printf("%c", BinaryData(vm, SymbolName(vm, val))[i]);
    }
    break;
  case BINARY:
    printf("\"");
    for (u32 i = 0; i < BinaryLength(vm, val); i++) {
      printf("%c", ((char*)BinaryData(vm, val))[i]);
    }
    printf("\"");
    break;
  case TUPLE:
    printf("[");
    for (u32 i = 0; i < TupleLength(vm, val); i++) {
      PrintValue(vm, TupleAt(vm, val, i));
      if (i < TupleLength(vm, val) - 1) printf(" ");
    }
    printf("]");
    break;
  default:
    printf("0x%08x", val.as_v);
  }
}
