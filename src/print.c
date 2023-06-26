#include "print.h"

#define PRINT_LIMIT 8

char *TypeAbbr(Val value, Mem *mem)
{
  if (IsNumeric(value) || IsNil(value)) return "";
  else if (IsSym(value))                return ":";
  else if (IsPair(value))               return "p";
  else if (IsTuple(mem, value))         return "t";
  else if (IsMap(mem, value))           return "m";
  else if (IsBinary(mem, value))        return "b";
  else if (IsBinaryHeader(value))       return "$";
  else if (IsTupleHeader(value))        return "#";
  else if (IsMapHeader(value))          return "%";
  else                                  return "?";
}

u32 PrintRawVal(Val value, Mem *mem)
{
  if (IsNum(value)) {
    return PrintFloat(RawNum(value), 3);
  } else if (IsInt(value)) {
    return PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    return PrintN(SymbolName(mem, value), SymbolLength(mem, value)) + 1;
  } else if (IsNil(value)) {
    return Print("nil");
  } else {
    Print(TypeAbbr(value, mem));
    return PrintInt(RawVal(value)) + 1;
  }
}

void PrintRawValN(Val value, u32 size, Mem *mem)
{
  if (IsNum(value)) {
    PrintFloatN(RawNum(value), size);
  } else if (IsInt(value)) {
    PrintIntN(RawInt(value), size, ' ');
  } else if (IsSym(value)) {
    if (Eq(value, SymbolFor("false"))) {
      PrintN("false", size);
    } else if (Eq(value, SymbolFor("true"))) {
      PrintN("true", size);
    } else {
      u32 len = Min(size-1, SymbolLength(mem, value));
      for (u32 i = 0; i < size - 1 - len; i++) Print(" ");
      Print(":");
      PrintN(SymbolName(mem, value), len);
    }
  } else if (IsNil(value)) {
    PrintN("nil", size);
  } else {
    u32 digits = NumDigits(RawVal(value));
    if (digits < size-1) {
      for (u32 i = 0; i < size-1-digits; i++) Print(" ");
    }

    Print(TypeAbbr(value, mem));
    PrintInt(RawVal(value));
  }
}

static u32 PrintSymbolVal(Val symbol, Mem *mem)
{
  u32 printed = 0;
  if (Eq(symbol, SymbolFor("true"))) {
    printed += Print("true");
  } else if (Eq(symbol, SymbolFor("false"))) {
    printed += Print("false");
  } else {
    printed += Print(":");
    printed += PrintSymbol(mem, symbol);
  }
  return printed;
}

static u32 PrintTuple(Val tuple, Mem *mem)
{
  u32 printed = 0;
  printed += Print("#[");
  u32 limit = TupleLength(mem, tuple);
  if (PRINT_LIMIT && PRINT_LIMIT < limit) limit = PRINT_LIMIT;
  for (u32 i = 0; i < limit; i++) {
    printed += PrintVal(mem, TupleAt(mem, tuple, i));
    if (i != TupleLength(mem, tuple) - 1) {
      printed += Print(", ");
    }
  }
  u32 rest = limit - TupleLength(mem, tuple);
  if (rest > 0) {
    printed += Print("...");
    printed += PrintInt(rest);
  }
  printed += Print("]");
  return printed;
}

static u32 PrintBinary(Val binary, Mem *mem)
{
  u32 printed = 0;
  u32 limit = BinaryLength(mem, binary);
  if (PRINT_LIMIT && PRINT_LIMIT*4 < limit) limit = PRINT_LIMIT*4;

  u8 *data = BinaryData(mem, binary);
  printed += Print("\"");
  for (u32 i = 0; i < limit; i++) {
    if (data[i] > 0x1F && data[i] < 0x7F) {
      printed += PrintChar(data[i]);
    } else {
      printed += Print(".");
    }
    printed += Print("\"");
  }

  return printed;
}

static u32 PrintTail(Mem *mem, Val tail, u32 max)
{
  u32 printed = 0;

  if (PRINT_LIMIT && max == 0) {
    printed += Print("...");
    printed += PrintInt(ListLength(mem, tail));
    printed += Print("]");
    return printed;
  }

  printed += PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    printed += Print("]");
  } else if (!IsPair(Tail(mem, tail))) {
    printed += Print(" | ");
    printed += PrintVal(mem, Tail(mem, tail));
    printed += Print("]");
  } else {
    printed += Print(" ");
    printed += PrintTail(mem, Tail(mem, tail), max - 1);
  }
  return printed;
}

u32 PrintVal(Mem *mem, Val value)
{
  u32 printed = 0;
  if (IsNil(value)) {
    printed += Print("nil");
  } else if (IsNum(value)) {
    printed += PrintFloat(value.as_f, 3);
  } else if (IsInt(value)) {
    printed += PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    printed += PrintSymbolVal(value, mem);
  } else if (IsPair(value)) {
    printed += Print("[");
    printed += PrintTail(mem, value, PRINT_LIMIT);
  } else if (IsTuple(mem, value)) {
    PrintTuple(value, mem);
  } else if (IsBinary(mem, value)) {
    u32 length = Min(16, BinaryLength(mem, value));
    u8 *data = BinaryData(mem, value);
    printed += Print("\"");
    for (u32 i = 0; i < length; i++) {
      if (data[i] > 31) {
        char c[2] = {data[i], '\0'};
        printed += Print(c);
      } else {
        printed += Print(".");
      }
    }
    printed += Print("\"");
  } else {
    printed += Print("<v");
    printed += PrintIntN(RawVal(value), 4, ' ');
    printed += Print(">");
  }

  return printed;
}

typedef struct TreePos {
  Val value;
  u32 position;
  struct TreePos *children;
} TreePos;

static TreePos LayoutTree(Val value, Mem *mem)
{
  TreePos pos = {value, 0, NULL};
  if (!IsList(mem, value)) return pos;

  pos.value = Head(mem, value);
  Val children = Tail(mem, value);
  while (!IsNil(children)) {
    VecPush(pos.children, LayoutTree(Head(mem, children), mem));
    children = Tail(mem, children);
  }

  return pos;
}

void PrintTree(Val value, Mem *mem)
{
  TreePos positions = LayoutTree(value, mem);

  TreePos **queue = NULL;
  Print(" ");
  PrintVal(mem, positions.value);
  Print("\n");
  if (positions.children != NULL) VecPush(queue, positions.children);

  while (VecCount(queue) > 0) {
    TreePos *row = VecPop(queue);
    for (u32 i = 0; i < VecCount(row); i++) {
      Print(" ");
      PrintVal(mem, row[i].value);
      if (row[i].children != NULL) VecPush(queue, row[i].children);
    }
    Print("\n");
  }
}
