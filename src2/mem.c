#include "mem.h"

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  mem->strings = NULL;
  InitMap(&mem->string_map);
  MakeSymbol("true", mem);
  MakeSymbol("false", mem);
  MakeSymbol("@", mem);
  MakeSymbol("λ", mem);
}

void DestoryMem(Mem *mem)
{
  DestroyMap(&mem->string_map);
  FreeVec(mem->strings);
  FreeVec(mem->values);
}

Val MakeSymbolFrom(char *name, u32 length, Mem *mem)
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
  return IsBinaryHeader(mem->values[index]);
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

static u32 DebugTail(Val value, Mem *mem)
{
  if (IsNil(value)) {
    return Print(")");
  }

  if (IsPair(value)) {
    u32 length = Print(" ");
    length += PrintVal(Head(value, mem), mem);
    return length + DebugTail(Tail(value, mem), mem);
  } else {
    u32 length = Print(" | ");
    length += PrintVal(value, mem);
    return length + Print(")");
  }
}

u32 PrintVal(Val value, Mem *mem)
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
    if (IsTagged(value, "λ", mem)) {
      Val num = ListAt(value, 1, mem);
      u32 length = Print("λ");
      length += PrintInt(RawInt(num));
      return length;
    }
    if (IsTagged(value, "@", mem)) {
      u32 length = Print("@");
      Val name = ListAt(value, 1, mem);
      length += Print(SymbolName(name, mem));
      return length;
    }
    if (IsTagged(value, "ε", mem)) {
      u32 length = Print("ε");
      u32 num = RawVal(value);
      length += PrintInt(num);
      return length;
    }

    u32 length = 0;
    length += Print("(");
    length += PrintVal(Head(value, mem), mem);
    return length + DebugTail(Tail(value, mem), mem);
  } else if (IsObj(value) && IsBinaryHeader(mem->values[RawVal(value)])) {
    Print("\"");
    PrintN(BinaryData(value, mem), BinaryLength(value, mem));
    Print("\"");
    return BinaryLength(value, mem) + 2;
  } else if (IsObj(value) && IsTupleHeader(mem->values[RawVal(value)])) {
    u32 length = Print("#[");
    for (u32 i = 0; i < TupleLength(value, mem); i++) {
      length += PrintVal(TupleGet(value, i, mem), mem);
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
        length += PrintVal(key, mem);
        length += Print(" : ");
      }
      length += PrintVal(TupleGet(values, i, mem), mem);
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

void PrintMem(Mem *mem)
{
  u32 ncols = 8;
  u32 stride = VecCount(mem->values) / ncols + 1;

  // as each row is printed, these store how many cells are left in an object, for each col
  u32 bins[ncols];
  u32 objs[ncols];
  for (u32 c = 0; c < ncols; c++) {
    bins[c] = 0;
    objs[c] = 0;
  }

  // since each col might start in the middle of an object, first scan through mem to mark the
  // starting values for the bins/objs arrays
  u32 i = 0;
  for (u32 c = 0; c < ncols-1 && i < VecCount(mem->values); c++) {
    u32 next_col = (c+1)*stride;
    while (i < next_col && i < VecCount(mem->values)) {
      u32 len = 2;
      if (IsTupleHeader(mem->values[i])) len = Max(1, RawVal(mem->values[i])) + 1;
      if (IsBinaryHeader(mem->values[i])) len = NumBinaryCells(RawVal(mem->values[i])) + 1;

      // if the length would go over a column, we initialize the column's byte/obj count
      if (i + len > next_col) {
        u32 left = (i+len) - next_col;
        if (IsBinaryHeader(mem->values[i])) {
          bins[c+1] = left;
        } else {
          objs[c+1] = left;
        }
      }
      i += len;
    }
  }

  // print header
  u32 actual_cols = Min(VecCount(mem->values), ncols);

  for (u32 c = 0; c < actual_cols; c++) {
    if (c == 0) Print("┏");
    else Print("┳");
    Print("━━━━┯━━━━━━━━━━");
  }
  Print("┓\n");

  for (u32 i = 0; i < stride; i++) {
    for (u32 c = 0; c < ncols; c++) {
      u32 n = i+c*stride;
      if (n < VecCount(mem->values)) {
        Print("┃");
        PrintIntN(n, 4, ' ');
        Print("│");

        Val value = mem->values[n];
        if (bins[c] > 0) {
          bins[c]--;

          char *data = (char*)(&value.as_i);
          if (IsPrintable(data[0]) && IsPrintable(data[1]) && IsPrintable(data[2]) && IsPrintable(data[3])) {
            Print("    \"");
            for (u32 ch = 0; ch < 4; ch++) PrintChar(data[ch]);
            Print("\"");
          } else {
            Print("  ");
            PrintHexN(value.as_i, 8, '0');
          }
        } else if (objs[c] > 0) {
          objs[c]--;

          PrintRawValN(mem->values[n], 10, mem);
        } else if (IsBinaryHeader(value)) {
          bins[c] = NumBinaryCells(RawVal(value));
          PrintRawValN(mem->values[n], 10, mem);
        } else if (IsTupleHeader(value)) {
          objs[c] = Max(1, RawVal(value));
          PrintRawValN(mem->values[n], 10, mem);
        } else {
          objs[c] = 1;
          PrintRawValN(mem->values[n], 10, mem);
        }
      } else {
        Print("┃    │          ");
      }
    }
    Print("┃\n");
  }

  // print footer
  for (u32 c = 0; c < actual_cols; c++) {
    if (c == 0) Print("┗");
    else Print("┻");
    Print("━━━━┷━━━━━━━━━━");
  }
  Print("┛\n");
}

