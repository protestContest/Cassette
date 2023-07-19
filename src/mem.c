#include "mem.h"
#include "proc.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitHashMap(&mem->string_map);
}

void DestroyMem(Mem *mem)
{
  DestroyHashMap(&mem->string_map);
  FreeVec(mem->strings);
  FreeVec(mem->values);
}

u32 MemSize(Mem *mem)
{
  return VecCount(mem->values);
}

Val MakeSymbolFrom(char *name, u32 length, Mem *mem)
{
  Val sym = SymbolFrom(name, length);
  if (HashMapContains(&mem->string_map, sym.as_i)) return sym;

  u32 index = VecCount(mem->strings);
  for (u32 i = 0; i < length && name[i] != '\0'; i++) {
    VecPush(mem->strings, name[i]);
  }
  VecPush(mem->strings, '\0');
  HashMapSet(&mem->string_map, sym.as_i, index);
  return sym;
}

Val MakeSymbol(char *name, Mem *mem)
{
  return MakeSymbolFrom(name, StrLen(name), mem);
}

char *SymbolName(Val symbol, Mem *mem)
{
  if (!HashMapContains(&mem->string_map, symbol.as_i)) return NULL;
  u32 index = HashMapGet(&mem->string_map, symbol.as_i);
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
  if (index >= VecCount(mem->values)) {
    Assert(index < VecCount(mem->values));
  }
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
  if (IsNil(b)) return a;

  Val cur = a;
  while (!IsNil(Tail(cur, mem))) {
    cur = Tail(cur, mem);
  }
  SetTail(cur, b, mem);
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

Val ListTake(Val list, u32 n, Mem *mem)
{
  Assert(IsPair(list));
  Val taken = nil;
  for (u32 i = 0; i < n && !IsNil(list); i++) {
    taken = Pair(Head(list, mem), taken, mem);
    list = Tail(list, mem);
  }

  return ReverseList(taken, mem);
}

Val ListAt(Val list, u32 pos, Mem *mem)
{
  if (pos == 0) return Head(list, mem);
  if (IsNil(list)) return nil;
  return ListAt(Tail(list, mem), pos-1, mem);
}

u32 ListLength(Val list, Mem *mem)
{
  Assert(IsPair(list));
  if (IsNil(list)) return 0;
  return ListLength(Tail(list, mem), mem) + 1;
}

Val ReverseList(Val list, Mem *mem)
{
  Assert(IsPair(list));

  Val new_list = nil;
  while (!IsNil(list)) {
    new_list = Pair(Head(list, mem), new_list, mem);
    list = Tail(list, mem);
  }
  return new_list;
}

Val ListFlatten(Val list, Mem *mem)
{
  Assert(IsPair(list));

  Val flattened = nil;

  while (!IsNil(list)) {
    Val item = Head(list, mem);

    if (IsNil(item)) {
      flattened = Pair(nil, flattened, mem);
    } else if (IsPair(item)) {
      item = ListFlatten(item, mem);
      while (!IsNil(item)) {
        flattened = Pair(Head(item, mem), flattened, mem);
        item = Tail(item, mem);
      }
    } else {
      flattened = Pair(item, flattened, mem);
    }

    list = Tail(list, mem);
  }

  return ReverseList(flattened, mem);
}

Val ListJoin(Val list, Val joiner, Mem *mem)
{
  Assert(IsPair(list));

  if (IsNil(list)) return BinaryFrom("", mem);

  Val iolist = Pair(Head(list, mem), nil, mem);
  list = Tail(list, mem);
  while (!IsNil(list)) {
    iolist = Pair(joiner, iolist, mem);
    iolist = Pair(Head(list, mem), iolist, mem);
    list = Tail(list, mem);
  }

  return ListToBinary(ReverseList(list, mem), mem);
}

static bool IOLength(Val list, u32 *length, Mem *mem)
{
  if (IsNil(list)) return true;

  Val item = Head(list, mem);
  if (IsBinary(item, mem)) {
    *length += BinaryLength(item, mem);
  } else if (IsInt(item)) {
    *length += 1;
  } else {
    return false;
  }

  return IOLength(Tail(list, mem), length, mem);
}

Val ListToBinary(Val list, Mem *mem)
{
  u32 length = 0;
  if (!IOLength(list, &length, mem)) return nil;

  Val bin = MakeBinary(length, mem);
  char *data = BinaryData(bin, mem);
  u32 i = 0;
  while (!IsNil(list)) {
    Val item = Head(list, mem);
    if (IsInt(item)) {
      data[i] = RawInt(item);
      i++;
    } else if (IsBinary(item, mem)) {
      char *item_data = BinaryData(item, mem);
      Copy(item_data, data + i, BinaryLength(item, mem));
      i += BinaryLength(item, mem);
    }
    list = Tail(list, mem);
  }

  return bin;
}

static u32 NumBinaryCells(u32 length)
{
  if (length == 0) return 1;
  return (length - 1) / sizeof(Val) + 1;
}

Val MakeBinary(u32 num_bytes, Mem *mem)
{
  u32 index = VecCount(mem->values);
  VecPush(mem->values, BinaryHeader(num_bytes));
  if (num_bytes == 0) {
    VecPush(mem->values, nil);
  } else {
    u32 words = NumBinaryCells(num_bytes);
    GrowVec(mem->values, words);
    char *data = (char*)(mem->values + index + 1);
    for (u32 i = 0; i < num_bytes; i++) {
      data[i] = 0;
    }
  }

  return ObjVal(index);
}

Val BinaryFrom(char *text, Mem *mem)
{
  u32 index = VecCount(mem->values);
  u32 length = StrLen(text);
  VecPush(mem->values, BinaryHeader(length));
  if (length == 0) {
    VecPush(mem->values, nil);
  } else {
    u32 words = NumBinaryCells(length);
    GrowVec(mem->values, words);
    char *data = (char*)(mem->values + index + 1);
    for (u32 i = 0; i < length; i++) {
      data[i] = text[i];
    }
  }

  return ObjVal(index);
}

bool IsBinary(Val bin, Mem *mem)
{
  u32 index = RawVal(bin);
  return IsObj(bin) && IsBinaryHeader(mem->values[index]);
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
  return IsObj(tuple) && IsTupleHeader(mem->values[RawVal(tuple)]);
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

Val TuplePut(Val tuple, u32 index, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  u32 obj_index = RawVal(tuple);
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);

  Val new_tuple = MakeTuple(length, mem);
  for (u32 i = 0; i < length; i++) {
    if (i == index) {
      TupleSet(new_tuple, i, value, mem);
    } else {
      TupleSet(new_tuple, i, TupleGet(tuple, i, mem), mem);
    }
  }

  return new_tuple;
}

Val TupleGet(Val tuple, u32 index, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  u32 obj_index = RawVal(tuple);
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  return mem->values[obj_index + 1 + index];
}

u32 TupleIndexOf(Val tuple, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));

  for (u32 i = 0; i < TupleLength(tuple, mem); i++) {
    if (Eq(value, TupleGet(tuple, i, mem))) {
      return i;
    }
  }

  return TupleLength(tuple, mem);
}

#define MAP_MIN_CAPACITY 8
Val MakeMap(u32 capacity, Mem *mem)
{
  u32 size = Max(capacity, MAP_MIN_CAPACITY);
  Val keys = MakeTuple(size, mem);
  Val values = MakeTuple(size, mem);

  return MapFrom(keys, values, mem);
}

Val MapFrom(Val keys, Val values, Mem *mem)
{
  u32 index = VecCount(mem->values);
  u32 count = 0;
  for (u32 i = 0; i < TupleLength(keys, mem); i++) {
    if (!IsNil(TupleGet(keys, i, mem))) count++;
  }

  VecPush(mem->values, MapHeader(count));
  VecPush(mem->values, keys);
  VecPush(mem->values, values);

  return ObjVal(index);
}

bool IsMap(Val map, Mem *mem)
{
  return IsObj(map) && IsMapHeader(mem->values[RawVal(map)]);
}

Val MapKeys(Val map, Mem *mem)
{
  Assert(IsMap(map, mem));
  u32 obj = RawVal(map);
  return mem->values[obj+1];
}

Val MapValues(Val map, Mem *mem)
{
  Assert(IsMap(map, mem));
  u32 obj = RawVal(map);
  return mem->values[obj+2];
}

u32 MapCapacity(Val map, Mem *mem)
{
  Assert(IsMap(map, mem));
  return TupleLength(MapKeys(map, mem), mem);
}

u32 MapCount(Val map, Mem *mem)
{
  Assert(IsMap(map, mem));
  u32 obj = RawVal(map);
  return RawVal(mem->values[obj]);
}

void MapSet(Val map, Val key, Val value, Mem *mem)
{
  Assert(IsMap(map, mem));

  Val keys = MapKeys(map, mem);
  Val vals = MapValues(map, mem);
  u32 index = TupleIndexOf(keys, key, mem);

  if (index >= TupleLength(keys, mem)) {
    index = MapCount(map, mem);
    mem->values[RawVal(map)] = MapHeader(MapCount(map, mem) + 1);
  }

  Assert(index < MapCapacity(map, mem));

  TupleSet(keys, index, key, mem);
  TupleSet(vals, index, value, mem);
}

Val MapPut(Val map, Val key, Val value, Mem *mem)
{
  Assert(IsMap(map, mem));

  if (MapContains(map, key, mem)) {
    // if the map already has this key/val pair, we're fine
    if (Eq(value, MapGet(map, key, mem))) return map;

    // otherwise, make a new map with new values set to this value
    Val keys = MapKeys(map, mem);
    Val vals = MapValues(map, mem);
    u32 index = TupleIndexOf(keys, key, mem);
    vals = TuplePut(vals, index, value, mem);
    return MapFrom(keys, vals, mem);
  }

  // map doesn't have key; create entirely new map
  u32 capacity = MapCapacity(map, mem);
  if (MapCount(map, mem) == capacity) capacity *= 2;

  Val new_map = MakeMap(capacity, mem);

  Val keys = MapKeys(map, mem);
  Val vals = MapValues(map, mem);
  for (u32 i = 0; i < MapCount(map, mem); i++) {
    MapSet(new_map, TupleGet(keys, i, mem), TupleGet(vals, i, mem), mem);
  }
  MapSet(new_map, key, value, mem);
  return new_map;
}

Val MapGet(Val map, Val key, Mem *mem)
{
  Assert(IsMap(map, mem));

  Val keys = MapKeys(map, mem);
  Val vals = MapValues(map, mem);

  u32 index = TupleIndexOf(keys, key, mem);
  if (index >= TupleLength(keys, mem)) return nil;

  return TupleGet(vals, index, mem);
}

Val MapDelete(Val map, Val key, Mem *mem)
{
  Assert(IsMap(map, mem));

  if (!MapContains(map, key, mem)) return map;

  Val keys = MapKeys(map, mem);
  Val vals = MapValues(map, mem);

  u32 index = TupleIndexOf(keys, key, mem);
  u32 count = MapCount(map, mem);

  Val new_map = MakeMap(count-1, mem);
  for (u32 i = 0; i < count; i++) {
    if (i == index) continue;
    MapPut(map, TupleGet(keys, i, mem), TupleGet(vals, i, mem), mem);
  }
  return new_map;
}

bool MapContains(Val map, Val key, Mem *mem)
{
  Assert(IsMap(map, mem));

  Val keys = MapKeys(map, mem);
  return TupleIndexOf(keys, key, mem) < TupleLength(keys, mem);
}

bool PrintVal(Val val, Mem *mem)
{
  if (IsBinary(val, mem)) {
    u32 length = BinaryLength(val, mem);
    PrintN(BinaryData(val, mem), length);
    return true;
  } else if (IsSym(val)) {
    Print(SymbolName(val, mem));
    return true;
  } else if (IsInt(val)) {
    PrintInt(RawInt(val));
    return true;
  } else if (IsNum(val)) {
    PrintFloat(RawNum(val), 3);
    return true;
  } else if (IsFunction(val, mem)) {
    Val num = ListAt(val, 1, mem);
    Print("λ");
    PrintInt(RawInt(num));
    return true;
  } else if (IsNil(val)) {
    return true;
  } else if (IsPair(val)) {
    if (IsTagged(val, "*env*", mem)) {
      Print("ε");
      u32 num = RawVal(val);
      PrintInt(num);
      return true;
    } else {
      return
        PrintVal(Head(val, mem), mem) &&
        PrintVal(Tail(val, mem), mem);
    }
  }

  return false;
}

static u32 InspectTail(Val value, Mem *mem)
{
  if (IsNil(value)) {
    return Print("]");
  }

  if (IsPair(value)) {
    u32 length = Print(" ");
    length += InspectVal(Head(value, mem), mem);
    return length + InspectTail(Tail(value, mem), mem);
  } else {
    u32 length = Print(" | ");
    length += InspectVal(value, mem);
    return length + Print("]");
  }
}

u32 InspectVal(Val value, Mem *mem)
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
  } else if (IsFunction(value, mem)) {
    u32 entry = FunctionEntry(value, mem);
    u32 length = Print("λ");
    length += PrintInt(entry);
    return length;
  } else if (IsPrimitive(value, mem)) {
    u32 length = Print("@");
    u32 num = PrimitiveNum(value, mem);
    length += PrintInt(num);
    return length;
  } else if (IsPair(value)) {
    if (IsTagged(value, "*env*", mem)) {
      u32 length = Print("ε");
      u32 num = RawVal(value);
      length += PrintInt(num);
      return length;
    }

    u32 length = 0;
    length += Print("[");
    length += InspectVal(Head(value, mem), mem);
    return length + InspectTail(Tail(value, mem), mem);
  } else if (IsObj(value) && IsBinaryHeader(mem->values[RawVal(value)])) {
    Print("\"");
    PrintN(BinaryData(value, mem), BinaryLength(value, mem));
    Print("\"");
    return BinaryLength(value, mem) + 2;
  } else if (IsObj(value) && IsTupleHeader(mem->values[RawVal(value)])) {
    u32 length = Print("{");
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      length += InspectVal(TupleGet(value, i, mem), mem);
      if (i < TupleLength(value, mem) - 1) length += Print(" ");
    }
    length += Print("}");
    return length;
  } else if (IsObj(value) && IsMapHeader(mem->values[RawVal(value)])) {
    u32 length = Print("{");
    Val keys = MapKeys(value, mem);
    Val values = MapValues(value, mem);
    for (u32 i = 0; i < MapCapacity(value, mem); i++) {
      Val key = TupleGet(keys, i, mem);
      if (IsNil(key)) break;

      if (i > 0) length += Print(", ");
      if (IsSym(key)) {
        length += Print(SymbolName(key, mem));
        length += Print(": ");
      } else {
        length += InspectVal(key, mem);
        length += Print(" : ");
      }
      length += InspectVal(TupleGet(values, i, mem), mem);
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

static char *TypeAbbr(Val value, Mem *mem)
{
  if (IsNumeric(value) || IsNil(value)) return "";
  else if (IsSym(value))                return ":";
  else if (IsPair(value))               return "p";
  else if (IsTuple(value, mem))         return "t";
  else if (IsMap(value, mem))           return "m";
  else if (IsBinary(value, mem))        return "b";
  else if (IsBinaryHeader(value))       return "$";
  else if (IsTupleHeader(value))        return "#";
  else if (IsMapHeader(value))          return "%";
  else                                  return "?";
}

static void PrintRawValN(Val value, u32 size, Mem *mem)
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
      u32 len = Min(size-1, NumChars(SymbolName(value, mem)));

      for (u32 i = 0; i < size - 1 - len; i++) Print(" ");
      Print(":");
      PrintN(SymbolName(value, mem), len);
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

static u32 ValSize(Val value)
{
  if (IsBinaryHeader(value)) {
    return NumBinaryCells(RawVal(value)) + 1;
  } else if (IsTupleHeader(value)) {
    return RawVal(value) + 1;
  } else if (IsMapHeader(value)) {
    return 3;
  } else {
    return 1;
  }
}

void PrintMem(Mem *mem)
{
  // divides the memory into 8 columns & print each column as a row

  u32 num_cols = 8;
  u32 stride = VecCount(mem->values) / num_cols + 1;

  // for each cell, we'll print the value in it. we have to keep track of
  // whether the current cell is part of a binary, so we can print that instead.
  // for each column, we'll track a count of how many cells are left of a binary
  u32 binaries[num_cols];
  for (u32 col = 0; col < num_cols; col++) binaries[col] = 0;

  // since each column might start in the middle of a binary, we first scan
  // through memory to find the initial values of each column
  for (u32 i = 0, col = 0; i < VecCount(mem->values) && col < num_cols-1;) {
    u32 next_col = stride*(col+1);

    u32 length = ValSize(mem->values[i]);
    if (IsBinaryHeader(mem->values[i]) && i + length > next_col) {
      // a binary straddles a column boundary; set the counter for that column
      u32 cells_left = i + length - next_col;
      binaries[col+1] = cells_left;
    }
    i += length;
  }

  // print header
  for (u32 c = 0; c < num_cols; c++) {
    if (c == 0) Print("┏");
    else Print("┳");
    Print("━━━━┯━━━━━━━━━━");
  }
  Print("┓\n");

  for (u32 i = 0; i < stride; i++) {
    for (u32 c = 0; c < num_cols; c++) {
      u32 n = i+c*stride;

      if (n >= VecCount(mem->values)) {
        // the last column might have blanks
        Print("┃    │          ");
        continue;
      }

      // print memory location
      Print("┃");
      PrintIntN(n, 4, ' ');
      Print("│");

      Val value = mem->values[n];
      if (IsBinaryHeader(value)) {
        // beginning of a binary; set the cell counter
        binaries[c] = NumBinaryCells(RawVal(value));
        PrintRawValN(mem->values[n], 10, mem);
      } else if (binaries[c] > 0) {
        // this cell is part of a binary
        binaries[c]--;

        char *data = (char*)(&value.as_i);
        if (IsPrintable(data[0]) && IsPrintable(data[1]) && IsPrintable(data[2]) && IsPrintable(data[3])) {
          // if the data is printable, assume it's a string
          Print("    \"");
          for (u32 ch = 0; ch < 4; ch++) PrintChar(data[ch]);
          Print("\"");
        } else {
          // otherwise print the hex value
          Print("  ");
          PrintHexN(value.as_i, 8, '0');
        }
      } else {
        PrintRawValN(mem->values[n], 10, mem);
      }
    }

    Print("┃\n");
  }

  // print footer
  for (u32 c = 0; c < num_cols; c++) {
    if (c == 0) Print("┗");
    else Print("┻");
    Print("━━━━┷━━━━━━━━━━");
  }
  Print("┛\n");
}

void PrintSymbols(Mem *mem)
{
  Print("Symbols:\n");
  for (u32 i = 0; i < HashMapCount(&mem->string_map); i++) {
    u32 index = GetHashMapValue(&mem->string_map, i);
    Print(":");
    Print(mem->strings + index);
    Print("\n");
  }
}

static Val CopyValue(Val value, Mem *old_mem, Mem *new_mem)
{
  if (IsNil(value)) return nil;

  if (IsPair(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("*moved*"))) {
      return old_mem->values[index+1];
    }

    u32 next = VecCount(new_mem->values);
    Val new_val = PairVal(next);
    VecPush(new_mem->values, old_mem->values[index]);
    VecPush(new_mem->values, old_mem->values[index+1]);

    old_mem->values[index] = SymbolFor("*moved*");
    old_mem->values[index+1] = new_val;

    return new_val;
  } else if (IsObj(value)) {
    u32 index = RawVal(value);
    if (Eq(old_mem->values[index], SymbolFor("*moved*"))) {
      return old_mem->values[index+1];
    }

    u32 next = VecCount(new_mem->values);
    Val new_val = ObjVal(next);

    for (u32 i = 0; i < ValSize(old_mem->values[index]); i++) {
      VecPush(new_mem->values, old_mem->values[index + i]);
    }

    old_mem->values[index] = SymbolFor("*moved*");
    old_mem->values[index+1] = new_val;
    return new_val;
  } else {
    return value;
  }
}

void CollectGarbage(Val *roots, Mem *mem)
{
  MakeSymbol("*moved*", mem);

  Val *new_vals = NewVec(Val, VecCount(mem->values) / 2);
  VecPush(new_vals, nil);
  VecPush(new_vals, nil);

  Mem new_mem = {new_vals, mem->strings, mem->string_map};
  u32 scan = VecCount(new_vals);

  for (u32 i = 0; i < VecCount(roots); i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  while (scan < VecCount(new_mem.values)) {
    if (IsBinaryHeader(new_mem.values[scan])) {
      scan += NumBinaryCells(RawVal(new_mem.values[scan])) + 1;
    } else {
      Val new_val = CopyValue(new_mem.values[scan], mem, &new_mem);
      new_mem.values[scan] = new_val;
      scan++;
    }
  }

  FreeVec(mem->values);
  mem->values = new_mem.values;
}
