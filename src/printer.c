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
  } else if (IsDict(val)) {
    printf("Map (%d)", (u32)RawVal(val));
  } else {
    printf("Obj (%d)", (u32)RawVal(val));
  }
}

u32 PrintValTo(Val val, char *dst, u32 start, u32 size);

u32 PrintTail(Val val, char *dst, u32 start, u32 size)
{
  start = PrintValTo(Head(val), dst, start, size);
  if (IsNil(Tail(val))) {
    return start;
  } else if (IsPair(Tail(val))) {
    start += snprintf(dst + start, size, " ");
    return PrintTail(Tail(val), dst, start, size);
  } else {
    start += snprintf(dst + start, size, " . ");
    return PrintValTo(Tail(val), dst, start, size);
  }
}

u32 PrintValTo(Val val, char *dst, u32 start, u32 size)
{
  if (IsNum(val)) {
    return start + snprintf(dst + start, size, "%.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    return start + snprintf(dst + start, size, "%d", (u32)RawVal(val));
  } else if (IsNil(val)) {
    return start + snprintf(dst + start, size, "nil");
  } else if (IsTagged(val, MakeSymbol("proc", 4))) {
    start += snprintf(dst + start, size, "[proc (");
    char *name = SymbolName(ProcName(val));
    start += snprintf(dst + start, size, "%s", name);
    Val params = ProcParams(val);
    while (!IsNil(params)) {
      start += snprintf(dst + start, size, " ");
      start = PrintValTo(Head(params), dst, start, size);
      params = Tail(params);
    }
    return start + snprintf(dst + start, size, ")]");
  } else if (IsPair(val)) {
    start += snprintf(dst + start, size, "(");
    start = PrintTail(val, dst, start, size);
    return start + snprintf(dst + start, size, ")");
  } else if (IsSym(val)) {
    char *name = SymbolName(val);
    return start + snprintf(dst + start, size, "%s", name);
  } else if (IsBin(val)) {
    u32 length = BinaryLength(val);
    char *data = BinaryData(val);
    if (size == 0) {
      return start + length + 2;
    } else {
      return start + snprintf(dst + start, length+3, "\"%s\"", data);
    }
  } else if (IsTuple(val)) {
    start += snprintf(dst + start, size, "[");
    u32 size = TupleLength(val);
    for (u32 i = 0; i < size; i++) {
      start = PrintValTo(TupleAt(val, i), dst, start, size);
      if (i != size-1) {
        start += snprintf(dst + start, size, " ");
      }
    }
    return start + snprintf(dst + start, size, "]");
  } else if (IsDict(val)) {
    start += snprintf(dst + start, size, "{");
    u32 size = DictSize(val);
    for (u32 i = 0; i < size; i++) {
      start = PrintValTo(DictKeyAt(val, i), dst, start, size);
      start += snprintf(dst + start, size, ": ");
      start = PrintValTo(DictValueAt(val, i), dst, start, size);
      if (i != size-1) {
        start += snprintf(dst + start, size, " ");
      }
    }
    return start + snprintf(dst + start, size, "}");
  } else {
    return start;
  }
}

u32 ValStrLen(Val val)
{
  return PrintValTo(val, NULL, 0, 0);
}

char *ValStr(Val val)
{
  u32 length = ValStrLen(val) + 1;
  char *str = malloc(length);
  PrintValTo(val, str, 0, length);
  return str;
}

void PrintVal(Val val)
{
  if (IsBin(val)) {
    u32 length = BinaryLength(val);
    char str[length];
    snprintf(str, length, "%s\n", BinaryData(val));
  } else if (IsSym(val)) {
    printf(":%s\n", SymbolName(val));
  } else {
    char *str = ValStr(val);
    printf("%s\n", str);
    free(str);
  }
}

void PrintTreeTail(Val exp, u32 indent)
{
  if (IsNil(exp)) return;

  if (!IsPair(exp)) {
    PrintTree(exp, indent);
  } else {
    PrintTree(Head(exp), indent);
    PrintTreeTail(Tail(exp), indent);
  }
}

#define Indent(n)   do { for (u32 i = 0; i < n; i++) printf("  "); } while (0)

void PrintTree(Val exp, u32 indent)
{
  if (indent > 10) {
    Error("Too much indent");
  }
  if (IsPair(exp) && !IsNil(exp)) {
    Indent(indent);
    DebugVal(exp);
    printf("\n");
    PrintTree(Head(exp), indent + 1);
    PrintTreeTail(Tail(exp), indent + 1);
  } else {
    Indent(indent);
    DebugVal(exp);
    printf("\n");
  }
}
