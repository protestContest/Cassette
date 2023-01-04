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

u32 ValStrLen(Val val);

u32 TailStrLen(Val val)
{
  u32 length = ValStrLen(Head(val));
  if (IsNil(Tail(val))) {
    return length;
  } else if (IsPair(Tail(val))) {
    length++;
    return length + TailStrLen(Tail(val));
  } else {
    length += 3;
    length += ValStrLen(Tail(val));
    return length;
  }
}

u32 ValStrLen(Val val)
{
  if (IsNum(val)) {
    return snprintf(NULL, 0, "%.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    return snprintf(NULL, 0, "%d", (u32)RawVal(val));
  } else if (IsNil(val)) {
    return 3;
  } else if (IsTagged(val, MakeSymbol("proc", 4))) {
    u32 length = 7;
    length += strlen(SymbolName(ProcName(val)));
    Val params = ProcParams(val);
    while (!IsNil(params)) {
      length++;
      length += ValStrLen(Head(params));
      params = Tail(params);
    }
    length += 2;
    return length;
  } else if (IsPair(val)) {
    return 2 + TailStrLen(val);
  } else if (IsSym(val)) {
    return strlen(SymbolName(val));
  } else if (IsBin(val)) {
    return snprintf(NULL, 0, "Bin (%d)", (u32)RawVal(val));
  } else if (IsTuple(val)) {
    u32 length = 1;
    u32 count = TupleLength(val);
    for (u32 i = 0; i < count; i++) {
      length += ValStrLen(TupleAt(val, i));
      if (i != count-1) {
        length++;
      }
    }
    length++;
    return length;
  } else if (IsDict(val)) {
    u32 length = 1;
    u32 size = DictSize(val);
    for (u32 i = 0; i < size; i++) {
      length += ValStrLen(DictKeyAt(val, i));
      length += 2;
      length += ValStrLen(DictValueAt(val, i));
      if (i != size-1) {
        length++;
      }
    }
    length++;
    return length;
  } else if (IsBin(val)) {
    return 2 + BinaryLength(val);
  }

  return 0;
}

char *PrintValTo(Val val, char *dst);

char *PrintTail(Val val, char *dst)
{
  dst = PrintValTo(Head(val), dst);
  if (IsNil(Tail(val))) {
    return dst;
  } else if (IsPair(Tail(val))) {
    dst += sprintf(dst, " ");
    return PrintTail(Tail(val), dst);
  } else {
    dst += sprintf(dst, " . ");
    return PrintValTo(Tail(val), dst);
  }
}

char *PrintValTo(Val val, char *dst)
{
  if (IsNum(val)) {
    return dst + sprintf(dst, "%.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    return dst + sprintf(dst, "%d", (u32)RawVal(val));
  } else if (IsNil(val)) {
    return dst + sprintf(dst, "nil");
  } else if (IsTagged(val, MakeSymbol("proc", 4))) {
    dst += sprintf(dst, "[proc (");
    char *name = SymbolName(ProcName(val));
    dst += sprintf(dst, "%s", name);
    Val params = ProcParams(val);
    while (!IsNil(params)) {
      dst += sprintf(dst, " ");
      dst = PrintValTo(Head(params), dst);
      params = Tail(params);
    }
    return dst + sprintf(dst, ")]");
  } else if (IsPair(val)) {
    dst += sprintf(dst, "(");
    dst = PrintTail(val, dst);
    return dst + sprintf(dst, ")");
  } else if (IsSym(val)) {
    char *name = SymbolName(val);
    return dst + sprintf(dst, "%s", name);
  } else if (IsBin(val)) {
    u32 length = BinaryLength(val);
    char *data = BinaryData(val);
    return dst + snprintf(dst, length+3, "\"%s\"", data);
  } else if (IsTuple(val)) {
    dst += sprintf(dst, "[");
    u32 size = TupleLength(val);
    for (u32 i = 0; i < size; i++) {
      dst = PrintValTo(TupleAt(val, i), dst);
      if (i != size-1) {
        dst += sprintf(dst, " ");
      }
    }
    return dst + sprintf(dst, "]");
  } else if (IsDict(val)) {
    dst += sprintf(dst, "{");
    u32 size = DictSize(val);
    for (u32 i = 0; i < size; i++) {
      dst = PrintValTo(DictKeyAt(val, i), dst);
      dst += sprintf(dst, ": ");
      dst = PrintValTo(DictValueAt(val, i), dst);
      if (i != size-1) {
        dst += sprintf(dst, " ");
      }
    }
    return dst + sprintf(dst, "}");
  } else {
    return dst;
  }
}

char *ValStr(Val val)
{
  char *str = malloc(ValStrLen(val) + 1);
  PrintValTo(val, str);
  return str;
}

void PrintVal(Val val)
{
  char *str = ValStr(val);
  printf("%s\n", str);
  free(str);
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
