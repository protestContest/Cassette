#include "mem.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitMap(&mem->string_map);
}

void DestoryMem(Mem *mem)
{
  DestroyMap(&mem->string_map);
  FreeVec(mem->strings);
  FreeVec(mem->values);
}

Val MakeSymbol(char *name, u32 length, Mem *mem)
{
  Val sym = SymbolFrom(name, length);
  u32 index = VecCount(mem->strings);
  for (u32 i = 0; i < length && *name != '\0'; i++) {
    VecPush(mem->strings, *name++);
  }
  VecPush(mem->strings, '\0');
  MapSet(&mem->string_map, sym.as_i, index);
  return sym;
}

char *SymbolName(Val symbol, Mem *mem)
{
  if (!MapContains(&mem->string_map, symbol.as_i)) return NULL;
  u32 index = MapGet(&mem->string_map, symbol.as_i);
  if (index >= VecCount(mem->strings)) return NULL;
  return mem->strings + index;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  u32 index = VecCount(mem->values);
  VecPush(mem->values, head);
  VecPush(mem->values, tail);
  return PairVal(index);
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index < VecCount(mem->values));
  return mem->values[index];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index < VecCount(mem->values)+1);
  return mem->values[index+1];
}

Val MakeBinary(char *text, Mem *mem)
{
  u32 index = VecCount(mem->values);
  u32 length = StrLen(text);
  VecPush(mem->values, BinaryHeader(length));
  if (length == 0) {
    VecPush(mem->values, nil);
  } else {
    u32 words = (length - 1) / 4 + 1;
    GrowVec(mem->values, words);
    char *data = (char*)(mem->values + index + 1);
    for (u32 i = 0; i < length; i++) {
      data[i] = text[i];
    }
  }

  return ObjVal(index);
}

u32 BinaryLength(Val bin, Mem *mem)
{
  u32 index = RawVal(bin);
  Assert(IsBinaryHeader(mem->values[index]));
  return RawVal(mem->values[index]);
}

char *BinaryData(Val bin, Mem *mem)
{
  u32 index = RawVal(bin);
  Assert(IsBinaryHeader(mem->values[index]));
  return (char*)(mem->values + index + 1);
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 index = VecCount(mem->values);
  VecPush(mem->values, TupleHeader(length));
  if (length == 0) {
    VecPush(mem->values, nil);
  } else {
    for (u32 i = 0; i < length; i++) {
      VecPush(mem->values, nil);
    }
  }
  return ObjVal(index);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  u32 obj_index = RawVal(tuple);
  Assert(IsTupleHeader(mem->values[obj_index]));
  return RawVal(mem->values[obj_index]);
}

void TupleSet(Val tuple, u32 index, Val value, Mem *mem)
{
  u32 obj_index = RawVal(tuple);
  Assert(IsTupleHeader(mem->values[obj_index]));
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  mem->values[obj_index + 1 + index] = value;
}

Val TupleGet(Val tuple, u32 index, Mem *mem)
{
  u32 obj_index = RawVal(tuple);
  Assert(IsTupleHeader(mem->values[obj_index]));
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  return mem->values[obj_index + 1 + index];
}

u32 DebugVal(Val value, Mem *mem)
{
  if (IsNil(value)) {
    return Print("nil");
  } else if (IsNum(value)) {
    return PrintFloat(value.as_f, 6);
  } else if (IsInt(value)) {
    return PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    return Print(SymbolName(value, mem)) + 1;
  } else if (IsPair(value)) {
    u32 length = 0;
    Print("(");
    length += DebugVal(Head(value, mem), mem);
    Print(" | ");
    length += DebugVal(Tail(value, mem), mem);
    Print(")");
    return length + 5;
  } else if (IsObj(value) && IsBinaryHeader(mem->values[RawVal(value)])) {
    Print("\"");
    PrintN(BinaryData(value, mem), BinaryLength(value, mem));
    Print("\"");
    return BinaryLength(value, mem) + 2;
  } else if (IsObj(value) && IsTupleHeader(mem->values[RawVal(value)])) {
    u32 length = Print("#[");
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      length += DebugVal(TupleGet(value, i, mem), mem);
      if (i < TupleLength(value, mem) - 1) length += Print(" ");
    }
    length += Print("]");
    return length;
  } else {
    u32 length = 0;
    Print("<?");
    length += PrintInt(value.as_i);
    Print(">");
    return length + 3;
  }
}
