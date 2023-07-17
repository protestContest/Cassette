#include "mem.h"
#include "proc.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitMap(&mem->string_map);
}

void DestroyMem(Mem *mem)
{
  DestroyMap(&mem->string_map);
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
  if (MapContains(&mem->string_map, sym.as_i)) return sym;

  u32 index = VecCount(mem->strings);
  for (u32 i = 0; i < length && name[i] != '\0'; i++) {
    VecPush(mem->strings, name[i]);
  }
  VecPush(mem->strings, '\0');
  MapSet(&mem->string_map, sym.as_i, index);
  return sym;
}

Val MakeSymbol(char *name, Mem *mem)
{
  return MakeSymbolFrom(name, StrLen(name), mem);
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

static u32 NumBinaryCells(u32 length)
{
  if (length == 0) return 1;
  return (length - 1) / sizeof(Val) + 1;
}

Val MakeBinary(char *text, Mem *mem)
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

Val TupleGet(Val tuple, u32 index, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  u32 obj_index = RawVal(tuple);
  u32 length = RawVal(mem->values[obj_index]);
  Assert(index < length);
  return mem->values[obj_index + 1 + index];
}

#define INIT_MAP_CAPACITY 8
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
  return IsObj(map) && IsMapHeader(mem->values[RawVal(map)]);
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
  u32 count = ValMapCount(map, mem);

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

  if (last_nil == cap) {
    // map full
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
  u32 index = RawVal(map);
  mem->values[index] = MapHeader(count + 1);
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
    u32 entry = ProcEntry(value, mem);
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
    u32 length = Print("#[");
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      length += InspectVal(TupleGet(value, i, mem), mem);
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
  else if (IsValMap(value, mem))        return "m";
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

void PrintSymbols(Mem *mem)
{
  Print("Symbols:\n");
  for (u32 i = 0; i < MapCount(&mem->string_map); i++) {
    u32 index = GetMapValue(&mem->string_map, i);
    Print(":");
    Print(mem->strings + index);
    Print("\n");
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
