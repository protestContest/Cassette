#include "debug.h"
#include "tuple.h"
#include "binary.h"
#include "list.h"
#include "symbol.h"
#include "map.h"

static u32 PeekVal(Val value, u32 size, Heap *mem)
{
  if (IsFloat(value)) {
    return PrintFloatN(RawFloat(value), size);
  } else if (IsInt(value)) {
    return PrintIntN(RawInt(value), size);
  } else if (IsSym(value)) {
    char *name = SymbolName(value, mem);
    if (name) {
      return PrintN(name, size);
    } else {
      Print("s");
      return PrintHexN(RawVal(value), 7) + 1;
    }
  } else if (IsNil(value)) {
    return PrintN("nil", size);
  } else if (IsPair(value)) {
    Print("p");
  } else if (IsBinaryHeader(value)) {
    Print("$");
  } else if (IsTupleHeader(value)) {
    Print("#");
  } else if (IsMapHeader(value)) {
    Print("%");
    return PrintHexN(RawVal(value), size-1) + 1;
  } else if (IsTuple(value, mem)) {
    Print("t");
  } else if (IsBinary(value, mem)) {
    Print("b");
  } else if (IsMap(value, mem)) {
    Print("m");
  } else {
    Print("?");
  }

  return PrintIntN(RawVal(value), size-1) + 1;
}

u32 DebugVal(Val value, Heap *mem)
{
  if (IsFloat(value)) {
    return PrintFloat(RawFloat(value), 3);
  } else if (IsInt(value)) {
    return PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    char *name = SymbolName(value, mem);
    if (name) {
      return Print(name);
    } else {
      Print("s");
      return PrintHexN(RawVal(value), 8) + 1;
    }
  } else if (IsNil(value)) {
    return Print("nil");
  } else if (IsPair(value)) {
    Print("p");
  } else if (IsBinaryHeader(value)) {
    Print("$");
  } else if (IsTupleHeader(value)) {
    Print("#");
  } else if (IsMapHeader(value)) {
    Print("%");
    return PrintHexN(RawVal(value), 8) + 1;
  } else if (IsTuple(value, mem)) {
    Print("t");
  } else if (IsBinary(value, mem)) {
    Print("b");
  } else if (IsMap(value, mem)) {
    Print("m");
  } else {
    Print("?");
  }

  return PrintInt(RawVal(value)) + 1;
}

static u32 InspectTail(Val value, Heap *mem)
{
  if (IsNil(value)) {
    return Print("]");
  }

  if (!IsPair(value)) {
    u32 len = Print(" | ");
    len += Inspect(value, mem);
    len += Print("]");
    return len;
  }

  u32 len = Inspect(Head(value, mem), mem);

  if (IsNil(Tail(value, mem))) {
    len += Print("]");
  } else {
    len += Print(" ");
    len += InspectTail(Tail(value, mem), mem);
  }

  return len;
}

u32 Inspect(Val value, Heap *mem)
{
  if (IsFloat(value)) {
    return PrintFloat(RawFloat(value), 2);
  } else if (IsInt(value)) {
    return PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    return Print(SymbolName(value, mem));
  } else if (IsNil(value)) {
    return Print("nil");
  } else if (IsPair(value)) {
    Print("[");
    return InspectTail(value, mem) + 1;
  } else if (IsTuple(value, mem)) {
    u32 len = Print("{");
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      len += Inspect(TupleGet(value, i, mem), mem);
      if (i < TupleLength(value, mem)-1) len += Print(" ");
    }
    len += Print("}");
    return len;
  } else if (IsBinary(value, mem)) {
    char *data = BinaryData(value, mem);
    u32 length = BinaryLength(value, mem);
    return PrintN(data, length);
  } else if (IsMap(value, mem)) {
    Print("{");
    u32 len = InspectMap(value, mem);
    Print("}");
    return len + 2;
  } else {
    return Print("?");
  }
}

void PrintMem(Heap *mem)
{
  u32 num_cols = 10;
  u32 col_size = 8;

  Print("╔");
  for (u32 i = 0; i < num_cols; i++) {
    for (u32 j = 0; j < 4; j++) Print("═");
    Print("╤");
    for (u32 j = 0; j < col_size; j++) Print("═");
    if (i < num_cols - 1) Print("╦");
  }
  Print("╗\n");

  for (u32 i = 0; i < MemSize(mem); i++) {
    Print("║");
    PrintIntN(i, 4);
    Print("│");
    u32 printed = PeekVal(mem->values[i], col_size, mem);
    if (printed < col_size) for (u32 j = printed; j < col_size; j++) Print(" ");

    if (i % num_cols == num_cols-1) {
      Print("║\n");
      Print("╠");
      for (u32 c = 0; c < num_cols; c++) {
        for (u32 j = 0; j < 4; j++) Print("═");
        Print("╪");
        for (u32 j = 0; j < col_size; j++) Print("═");
        if (c < num_cols - 1) Print("╬");
      }
      Print("╣\n");
    }
  }

  u32 empty_cols = num_cols - MemSize(mem) % num_cols;
  for (u32 i = 0; i < empty_cols; i++) {
    Print("║    │        ");
  }
  Print("║\n");
  Print("╚");
  for (u32 i = 0; i < num_cols; i++) {
    for (u32 j = 0; j < 4; j++) Print("═");
    Print("╧");
    for (u32 j = 0; j < col_size; j++) Print("═");
    if (i < num_cols - 1) Print("╩");
  }
  Print("╝\n");
}
