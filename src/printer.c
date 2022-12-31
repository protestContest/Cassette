#include "printer.h"
#include "mem.h"
#include "proc.h"

void DebugVal(Val val)
{
  if (IsNil(val)) {
    printf("nil (%d)", (u32)RawVal(val));
  } else if (IsNum(val)) {
    printf("%.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    printf("%d", (u32)RawVal(val));
  } else if (IsPair(val)) {
    printf("Pair (%d)", (u32)RawVal(val));
  } else if (IsSym(val)) {
    char *name = SymbolName(val);
    printf("%s", name);
  } else if (IsBin(val)) {
    printf("Bin (%d)", (u32)RawVal(val));
  } else if (IsTuple(val)) {
    printf("Tuple (%d)", (u32)RawVal(val));
  } else if (IsHdr(val)) {
    printf("Obj (%d)", (u32)RawVal(val));
  }
}

void PrintTail(Val val)
{
  PrintVal(Head(val));
  if (IsNil(Tail(val))) {
    return;
  } else if (IsPair(Tail(val))) {
    printf(" ");
    PrintTail(Tail(val));
  } else {
    printf(" . ");
    PrintVal(Tail(val));
  }
}

void PrintVal(Val val)
{
  if (IsNum(val)) {
    printf("%.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    printf("%d", (u32)RawVal(val));
  } else if (IsNil(val)) {
    printf("nil");
  } else if (IsTagged(val, MakeSymbol("proc", 4))) {
    printf("[proc");
    Val params = ProcParams(val);
    while (!IsNil(params)) {
      printf(" ");
      PrintVal(Head(params));
      params = Tail(params);
    }
    printf("]");
  } else if (IsPair(val)) {
    printf("(");
    PrintTail(val);
    printf(")");
  } else if (IsSym(val)) {
    char *name = SymbolName(val);
    printf("%s", name);
  } else if (IsBin(val)) {
    printf("Bin (%d)", (u32)RawVal(val));
  } else if (IsTuple(val)) {
    printf("[");
    u32 size = RawVal(TupleLength(val));
    for (u32 i = 0; i < size; i++) {
      PrintVal(TupleAt(val, i));
      if (i != size-1) {
        printf(" ");
      }
    }
    printf("]");
  } else if (IsHdr(val)) {
    printf("Obj (%d)", (u32)RawVal(val));
  }
}
