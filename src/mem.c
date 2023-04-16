#include "mem.h"
#include <stdarg.h>

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  InitMap(&mem->symbols);
}

Val MakePair(Mem *mem, Val head, Val tail)
{
  Val pair = PairVal(VecCount(mem->values));

  VecPush(mem->values, head);
  VecPush(mem->values, tail);

  return pair;
}

Val Head(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawPair(pair);
  return mem->values[index];
}

Val Tail(Mem *mem, Val pair)
{
  if (IsNil(pair)) return nil;
  u32 index = RawPair(pair);
  return mem->values[index+1];
}

void SetHead(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  mem->values[index] = val;
}

void SetTail(Mem *mem, Val pair, Val val)
{
  Assert(!IsNil(pair));
  u32 index = RawPair(pair);
  mem->values[index+1] = val;
}

Val MakeList(Mem *mem, u32 num, ...)
{
  va_list args;
  va_start(args, num);
  Val list = nil;
  for (u32 i = 0; i < num; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(mem, arg, list);
  }
  va_end(args);
  return ReverseOnto(mem, list, nil);
}

u32 ListLength(Mem *mem, Val list)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(mem, list);
  }
  return length;
}

Val ListAt(Mem *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return Head(mem, list);
  return ListAt(mem, Tail(mem, list), index - 1);
}

Val ReverseOnto(Mem *mem, Val list, Val tail)
{
  if (IsNil(list)) return tail;
  return ReverseOnto(mem, Tail(mem, list), MakePair(mem, Head(mem, list), tail));
}

Val ListAppend(Mem *mem, Val list1, Val list2)
{
  list1 = ReverseOnto(mem, list1, nil);
  return ReverseOnto(mem, list1, list2);
}

bool IsTagged(Mem *mem, Val list, Val tag)
{
  if (!IsPair(list)) return false;
  return Eq(Head(mem, list), tag);
}

bool IsTuple(Mem *mem, Val tuple)
{
  u32 index = RawObj(tuple);
  return HeaderType(mem->values[index]) == tupleMask;
}

Val MakeTuple(Mem *mem, u32 count)
{
  Val tuple = ObjVal(VecCount(mem->values));
  VecPush(mem->values, TupleHeader(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(mem->values, nil);
  }

  return tuple;
}

u32 TupleLength(Mem *mem, Val tuple)
{
  u32 index = RawObj(tuple);
  return HeaderVal(mem->values[index]);
}

Val TupleAt(Mem *mem, Val tuple, u32 i)
{
  u32 index = RawObj(tuple);
  return mem->values[index + i + 1];
}

void TupleSet(Mem *mem, Val tuple, u32 i, Val val)
{
  u32 index = RawObj(tuple);
  mem->values[index + i + 1] = val;
}

bool IsDict(Mem *mem, Val dict)
{
  u32 index = RawObj(dict);
  return HeaderType(mem->values[index]) == dictMask;
}

Val MakeDict(Mem *mem, Val keys, Val vals)
{
  u32 count = TupleLength(mem, keys);
  Val dict = ObjVal(VecCount(mem->values));
  VecPush(mem->values, DictHeader(count));
  VecPush(mem->values, keys);
  VecPush(mem->values, vals);

  return dict;
}

u32 DictSize(Mem *mem, Val dict)
{
  u32 index = RawObj(dict);
  return HeaderVal(mem->values[index]);
}

Val DictKeys(Mem *mem, Val dict)
{
  u32 index = RawObj(dict);
  return mem->values[index+1];
}

Val DictValues(Mem *mem, Val dict)
{
  u32 index = RawObj(dict);
  return mem->values[index+2];
}

bool InDict(Mem *mem, Val dict, Val key)
{
  u32 index = RawObj(dict);
  u32 count = DictSize(mem, dict);
  Val keys = mem->values[index+1];
  for (u32 i = 0; i < count; i++) {
    if (Eq(TupleAt(mem, keys, i), key)) {
      return true;
    }
  }
  return false;
}

Val DictGet(Mem *mem, Val dict, Val key)
{
  u32 index = RawObj(dict);
  u32 count = DictSize(mem, dict);
  Val keys = mem->values[index+1];
  Val vals = mem->values[index+2];
  for (u32 i = 0; i < count; i++) {
    if (Eq(TupleAt(mem, keys, i), key)) {
      return TupleAt(mem, vals, i);
    }
  }
  return nil;
}

Val DictSet(Mem *mem, Val dict, Val key, Val value)
{
  u32 index = RawObj(dict);
  u32 count = DictSize(mem, dict);
  Val keys = mem->values[index+1];
  Val vals = mem->values[index+2];

  if (InDict(mem, dict, key)) {
    Val new_vals = MakeTuple(mem, count);
    for (u32 i = 0; i < count; i++) {
      if (Eq(TupleAt(mem, keys, i), key)) {
        TupleSet(mem, new_vals, i, value);
        for (u32 j = i+1; j < count; j++) {
          TupleSet(mem, new_vals, j, TupleAt(mem, vals, j));
        }
        return MakeDict(mem, keys, new_vals);
      }
      TupleSet(mem, new_vals, i, TupleAt(mem, vals, i));
    }
    return nil;
  } else {
    Val new_keys = MakeTuple(mem, count+1);
    Val new_vals = MakeTuple(mem, count+1);
    for (u32 i = 0; i < count; i++) {
      TupleSet(mem, new_keys, i, TupleAt(mem, keys, i));
      TupleSet(mem, new_vals, i, TupleAt(mem, vals, i));
    }
    TupleSet(mem, new_keys, count, key);
    TupleSet(mem, new_vals, count, value);
    return MakeDict(mem, new_keys, new_vals);
  }
}

Val MakeSymbolFrom(Mem *mem, char *str, u32 length)
{
  Val sym = SymbolFrom(str, length);
  if (!MapContains(&mem->symbols, sym.as_v)) {
    char *sym_str = Allocate(length + 1);
    for (u32 i = 0; i < length; i++) sym_str[i] = str[i];
    sym_str[length] = '\0';
    MapSet(&mem->symbols, sym.as_v, sym_str);
  }
  return sym;
}

Val MakeSymbol(Mem *mem, char *str)
{
  return MakeSymbolFrom(mem, str, StrLen(str));
}

char *SymbolName(Mem *mem, Val symbol)
{
  return MapGet(&mem->symbols, symbol.as_v);
}

bool IsBinary(Mem *mem, Val binary)
{
  u32 index = RawObj(binary);
  return HeaderType(mem->values[index]) == binaryMask;
}

Val MakeBinaryFrom(Mem *mem, char *str, u32 length)
{
  Val binary = ObjVal(VecCount(mem->values));
  VecPush(mem->values, BinaryHeader(length));

  u32 num_words = (length - 1) / sizeof(Val) + 1;
  GrowVec(mem->values, num_words);

  u8 *bytes = BinaryData(mem, binary);
  for (u32 i = 0; i < length; i++) {
    bytes[i] = str[i];
  }

  return binary;
}

u32 BinaryLength(Mem *mem, Val binary)
{
  u32 index = RawObj(binary);
  return HeaderVal(mem->values[index]);
}

u8 *BinaryData(Mem *mem, Val binary)
{
  u32 index = RawObj(binary);
  return (u8*)(mem->values + index + 1);
}

static u32 PrintTail(Mem *mem, Val tail, u32 length)
{
  length += PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    Print("]");
    return length + 1;
  }
  if (!IsPair(Tail(mem, tail))) {
    Print(" | ");
    length += 3;
    length += PrintVal(mem, Tail(mem, tail));
    Print("]");
    return length + 1;
  }
  Print(" ");
  return PrintTail(mem, Tail(mem, tail), length + 1);
}

u32 PrintVal(Mem *mem, Val value)
{
  if (IsNil(value)) {
    Print("nil");
    return 3;
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
    return NumDigits(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value));
    return NumDigits(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    char *str = SymbolName(mem, value);
    Print(str);
    return StrLen(str) + 1;
  } else if (IsPair(value)) {
    if (IsTagged(mem, value, SymbolFor("proc"))) {
      Print("[λ");
      Val params = ListAt(mem, value, 1);
      while (!IsNil(params)) {
        Print(" ");
        Print(SymbolName(mem, Head(mem, params)));
        params = Tail(mem, params);
      }
      Print("]");
      return 0;
    } else {
      Print("[");
      return PrintTail(mem, value, 1);
    }
  } else if (IsTuple(mem, value)) {
    Print("#[");
    for (u32 i = 0; i < TupleLength(mem, value); i++) {
      PrintVal(mem, TupleAt(mem, value, i));
      if (i != TupleLength(mem, value) - 1) {
        Print(" ");
      }
    }
    Print("]");
    return 0;
  } else if (IsDict(mem, value)) {
    Print("{");
    Val keys = DictKeys(mem, value);
    Val vals = DictValues(mem, value);
    for (u32 i = 0; i < DictSize(mem, value); i++) {
      Val key = TupleAt(mem, keys, i);
      if (IsSym(key)) {
        Print(SymbolName(mem, key));
        Print(": ");
      } else {
        PrintVal(mem, key);
        Print(" => ");
      }
      PrintVal(mem, TupleAt(mem, vals, i));
      if (i != TupleLength(mem, value) - 1) {
        Print(" ");
      }
    }
    Print("}");
    return 0;
  } else if (IsBinary(mem, value)) {
    u32 length = Min(16, BinaryLength(mem, value));
    u8 *data = BinaryData(mem, value);
    Print("\"");
    for (u32 i = 0; i < length; i++) {
      if (data[i] > 31) {
        char c[2] = {data[i], '\0'};
        Print(c);
      } else {
        Print(".");
      }
    }
    Print("\"");
    return 0;
  } else {
    Print("<v");
    PrintIntN(RawObj(value), 4);
    Print(">");
    return 7;
  }
}

void DebugVal(Mem *mem, Val value)
{
  if (IsNil(value)) {
    Print("nil");
  } else if (IsNum(value)) {
    PrintFloat(value.as_f);
  } else if (IsInt(value)) {
    PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    Print(":");
    char *str = SymbolName(mem, value);
    Print(str);
  } else if (IsPair(value)) {
    Print("p");
    PrintInt(RawPair(value));
  } else {
    Print("<v");
    PrintIntN(RawObj(value), 4);
    Print(">");
  }
}

void PrintMem(Mem *mem)
{
  for (u32 i = 0; i < VecCount(mem->values); i++) {
    Print("[");
    PrintIntN(i, 4);
    Print("] ");
    DebugVal(mem, mem->values[i]);
    Print("\n");
  }
}
