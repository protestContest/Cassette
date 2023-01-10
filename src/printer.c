#include "printer.h"
#include "mem.h"
#include "proc.h"

void DebugVal(Val val)
{
  if (IsNil(val)) {
    printf(" nil");
  } else if (IsNum(val)) {
    printf("%-4.1f", (float)RawVal(val));
  } else if (IsInt(val)) {
    printf("%-4d", (i32)RawVal(val));
  } else if (IsPair(val)) {
    printf("p%3d", (i32)RawVal(val));
  } else if (IsSym(val)) {
    char *name = SymbolName(val);
    printf(":%s", name);
  } else if (IsBin(val)) {
    printf("b%-3d", (i32)RawVal(val));
  } else if (IsTuple(val)) {
    printf("t%-3d", (i32)RawVal(val));
  } else if (IsDict(val)) {
    printf("m%-3d", (i32)RawVal(val));
  } else {
    printf("?%-3d", (i32)RawVal(val));
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
  str[length] = '\0';
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

void Indent(u32 level, u32 lines, bool end)
{
  for (u32 i = 0; i < level; i++) {
    u32 line = (0x1 << i) & lines;

    if (i == level-1) {
      if (end) {
        printf("└");
      } else {
        printf("├");
      }
    } else if (line) {
      printf("│");
    } else {
      printf(" ");
    }
  }
}

void PrintTreeLevel(Val exp, u32 level, u32 lines);

void PrintTreeTail(Val exp, u32 level, u32 lines)
{
  if (IsNil(Tail(exp))) {
    Indent(level, lines, true);
    lines = lines & ~(0x1 << (level-1));
    PrintTreeLevel(Head(exp), level, lines);
  } else {
    Indent(level, lines, false);
    PrintTreeLevel(Head(exp), level, lines);

    if (IsPair(Tail(exp))) {
      PrintTreeTail(Tail(exp), level, lines);
    } else {
      Indent(level, lines, true);
      PrintTreeLevel(Tail(exp), level, lines);
    }
  }
}

void PrintTreeLevel(Val exp, u32 level, u32 lines)
{
  if (level > 32) Error("Too much indent");

  if (IsPair(exp) && !IsNil(exp) && IsPair(Tail(exp))) {
    printf("┬");
    lines = (0x1 << level) | lines;
    level++;
    PrintTreeLevel(Head(exp), level, lines);
    PrintTreeTail(Tail(exp), level, lines);
  } else {
    printf("╴");
    if (IsPair(exp)) {
      printf("(");
      DebugVal(Head(exp));
      printf(" | ");
      DebugVal(Tail(exp));
      printf(")\n");
    } else {
      DebugVal(exp);
      printf("\n");
    }
  }
}

void PrintTree(Val exp)
{
  Indent(0, 0, false);
  PrintTreeLevel(exp, 0, 0);
}
