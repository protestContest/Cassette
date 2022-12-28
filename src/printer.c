#include "printer.h"
#include "list.h"

void PrintValueTo(VM *vm, Val val, FILE *stream);

void PrintTail(VM *vm, Val tail, FILE *stream)
{
  static u32 n = 0;
  if (n++ > 1000) {
    exit(1);
  }

  if (IsNil(tail)) {
    fprintf(stream, ")");
    return;
  }

  if (IsLoop(vm, tail)) {
    fprintf(stream, " ^)");
    return;
  }

  fprintf(stream, " ");
  PrintValueTo(vm, Head(vm, tail), stream);
  PrintTail(vm, Tail(vm, tail), stream);
}

void PrintValueTo(VM *vm, Val val, FILE *stream)
{
  switch (TypeOf(val)) {
  case NUMBER:
    fprintf(stream, "%.1f", (float)RawVal(val));
    break;
  case INTEGER:
    fprintf(stream, "%d", (u32)RawVal(val));
    break;
  case PAIR:
    fprintf(stream, "(");
    if (IsNil(val)) {
      fprintf(stream, ")");
    } else {
      if (Eq(val, Head(vm, val))) {
        fprintf(stream, "_");
      } else {
        PrintValueTo(vm, Head(vm, val), stream);
      }

      if (Eq(val, Tail(vm, val))) {
        fprintf(stream, " . ^)");
      } else if (IsPair(Tail(vm, val))) {
        if (Eq(Head(vm, val), MakeSymbol(vm, "proc", 4))) {
          fprintf(stream, " ...)");
        } else {
          PrintTail(vm, Tail(vm, val), stream);
        }
      } else {
        fprintf(stream, " . ");
        PrintValueTo(vm, Tail(vm, val), stream);
        fprintf(stream, ")");
      }
    }
    break;
  case SYMBOL:
    for (u32 i = 0; i < BinaryLength(vm, SymbolName(vm, val)); i++) {
      fprintf(stream, "%c", BinaryData(vm, SymbolName(vm, val))[i]);
    }
    break;
  case BINARY:
    fprintf(stream, "\"");
    for (u32 i = 0; i < BinaryLength(vm, val); i++) {
      fprintf(stream, "%c", ((char*)BinaryData(vm, val))[i]);
    }
    fprintf(stream, "\"");
    break;
  case TUPLE:
    fprintf(stream, "[");
    for (u32 i = 0; i < TupleLength(vm, val); i++) {
      PrintValueTo(vm, TupleAt(vm, val, i), stream);
      if (i < TupleLength(vm, val) - 1) fprintf(stream, " ");
    }
    fprintf(stream, "]");
    break;
  default:
    fprintf(stream, "0x%08x", val.as_v);
  }
}

void PrintValue(VM *vm, Val val)
{
  PrintValueTo(vm, val, stdout);
}

void DebugValue(u32 level, VM *vm, Val val) {
  if (DEBUG && level) {
    fprintf(stderr, "; ");
    PrintValueTo(vm, val, stderr);
    fprintf(stderr, "\n");
  }
}
