#include "mem.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitMap(&mem->string_map);
  MakeSymbol("true", 4, mem);
  MakeSymbol("false", 5, mem);
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
  Assert(index+1 < VecCount(mem->values));
  return mem->values[index+1];
}

void SetHead(Val pair, Val head, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index < VecCount(mem->values));
  mem->values[index] = head;
}

void SetTail(Val pair, Val tail, Mem *mem)
{
  Assert(IsPair(pair));
  u32 index = RawVal(pair);
  Assert(index+1 < VecCount(mem->values));
  mem->values[index+1] = tail;
}

bool ListContains(Val list, Val value, Mem *mem)
{
  Assert(IsPair(list));

  while (!IsNil(list)) {
    if (Eq(value, Head(list, mem))) {
      return true;
    }
    list = Tail(list, mem);
  }
  return false;
}

Val ListConcat(Val a, Val b, Mem *mem)
{
  Assert(IsPair(a));
  if (IsNil(a)) return b;
  while (!IsNil(Tail(a, mem))) a = Tail(a, mem);
  SetTail(a, b, mem);
  return a;
}

bool IsTagged(Val list, char *tag, Mem *mem)
{
  if (!IsPair(list)) return false;
  return Eq(SymbolFor(tag), Head(list, mem));
}

Val ListFrom(Val list, u32 pos, Mem *mem)
{
  Assert(IsPair(list));
  for (u32 i = 0; i < pos; i++) {
    list = Tail(list, mem);
  }
  return list;
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

bool IsTuple(Val tuple, Mem *mem)
{
  return IsTupleHeader(mem->values[RawVal(tuple)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal(mem->values[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  for (u32 i = 0; i < TupleLength(tuple, mem); i++) {
    if (Eq(value, TupleGet(tuple, i, mem))) {
      return true;
    }
  }
  return false;
}

void TupleSet(Val tuple, u32 index, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  u32 obj_index = RawVal(tuple);
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  mem->values[obj_index + 1 + index] = value;
}

Val TupleGet(Val tuple, u32 index, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  u32 obj_index = RawVal(tuple);
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  return mem->values[obj_index + 1 + index];
}

#define INIT_MAP_CAPACITY 32
Val MakeValMap(Mem *mem)
{
  u32 size = INIT_MAP_CAPACITY;
  Val keys = MakeTuple(size, mem);
  Val values = MakeTuple(size, mem);

  u32 index = VecCount(mem->values);
  VecPush(mem->values, MapHeader(0));
  VecPush(mem->values, keys);
  VecPush(mem->values, values);

  return ObjVal(index);
}

bool IsValMap(Val map, Mem *mem)
{
  return IsMapHeader(mem->values[RawVal(map)]);
}

Val ValMapKeys(Val map, Mem *mem)
{
  Assert(IsValMap(map, mem));
  u32 obj = RawVal(map);
  return mem->values[obj+1];
}

Val ValMapValues(Val map, Mem *mem)
{
  Assert(IsValMap(map, mem));
  u32 obj = RawVal(map);
  return mem->values[obj+2];
}

u32 ValMapCapacity(Val map, Mem *mem)
{
  Assert(IsValMap(map, mem));
  return TupleLength(ValMapKeys(map, mem), mem);
}

u32 ValMapCount(Val map, Mem *mem)
{
  Assert(IsValMap(map, mem));
  u32 obj = RawVal(map);
  return RawVal(mem->values[obj]);
}

void ValMapSet(Val map, Val key, Val value, Mem *mem)
{
  Assert(IsValMap(map, mem));

  u32 cap = ValMapCapacity(map, mem);
  Val keys = ValMapKeys(map, mem);
  Val vals = ValMapValues(map, mem);
  u32 last_nil = cap;
  for (u32 i = 0; i < cap; i++) {
    if (Eq(key, TupleGet(keys, i, mem))) {
      TupleSet(vals, i, value, mem);
      return;
    } else if (IsNil(TupleGet(keys, i, mem))) {
      last_nil = i;
      break;
    }
  }

  // map full
  if (last_nil == cap) {
    Val new_keys = MakeTuple(2*cap, mem);
    Val new_vals = MakeTuple(2*cap, mem);
    for (u32 i = 0; i < cap; i++) {
      TupleSet(new_keys, i, TupleGet(keys, i, mem), mem);
      TupleSet(new_vals, i, TupleGet(vals, i, mem), mem);
    }
    for (u32 i = cap; i < 2*cap; i++) {
      TupleSet(new_keys, i, nil, mem);
      TupleSet(new_vals, i, nil, mem);
    }

    mem->values[RawVal(map)+1] = new_keys;
    mem->values[RawVal(map)+2] = new_vals;
    keys = new_keys;
    vals = new_vals;
  }

  TupleSet(keys, last_nil, key, mem);
  TupleSet(vals, last_nil, value, mem);
}

void ValMapPut(Val map, Val key, Val value, Mem *mem)
{
  Assert(IsValMap(map, mem));

  if (ValMapContains(map, key, mem) && Eq(value, ValMapGet(map, key, mem))) return;

  u32 cap = ValMapCapacity(map, mem);
  if (ValMapCount(map, mem) == cap && !ValMapContains(map, key, mem)) {
    cap = 2*cap;
  }

  Val keys = ValMapKeys(map, mem);
  Val vals = ValMapValues(map, mem);
  Val new_vals = MakeTuple(cap, mem);
  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    if (Eq(key, TupleGet(keys, i, mem))) {
      TupleSet(new_vals, i, value, mem);
    } else {
      TupleSet(new_vals, i, TupleGet(vals, i, mem), mem);
    }
  }
  for (u32 i = TupleLength(keys, mem); i < cap; i++) {
    TupleSet(new_vals, i, nil, mem);
  }
}

Val ValMapGet(Val map, Val key, Mem *mem)
{
  u32 obj = RawVal(map);
  Assert(IsMapHeader(mem->values[obj]));
  Val keys = mem->values[obj+1];
  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    if (Eq(key, TupleGet(keys, i, mem))) {
      Val vals = mem->values[obj+2];
      return TupleGet(vals, i, mem);
    }
  }
  return nil;
}

bool ValMapContains(Val map, Val key, Mem *mem)
{
  Assert(IsValMap(map, mem));

  Val keys = mem->values[RawVal(map)+1];
  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    if (Eq(key, TupleGet(keys, i, mem))) {
      return true;
    }
  }
  return false;
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
    if (IsPair(Tail(value, mem))) {
      length += Print("...");
    } else {
      length += DebugVal(Tail(value, mem), mem);
    }
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
  } else if (IsObj(value) && IsMapHeader(mem->values[RawVal(value)])) {
    u32 length = Print("{");
    Val keys = ValMapKeys(value, mem);
    Val values = ValMapValues(value, mem);
    for (u32 i = 0; i < ValMapCapacity(value, mem); i++) {
      Val key = TupleGet(keys, i, mem);
      if (IsNil(key)) break;

      if (i > 0) length += Print(", ");
      if (IsSym(key)) {
        length += Print(SymbolName(key, mem));
        length += Print(": ");
      } else {
        length += DebugVal(key, mem);
        length += Print(" : ");
      }
      length += DebugVal(TupleGet(values, i, mem), mem);
    }
    length += Print("}");
    return length;
  } else {
    u32 length = 0;
    Print("<?");
    length += PrintInt(value.as_i);
    Print(">");
    return length + 3;
  }
}
