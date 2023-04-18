#include "mem.h"
#include <stdarg.h>

#define Free(mem)   VecCount(mem->values)

void InitMem(Mem *mem)
{
  mem->values = NULL;
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  InitMap(&mem->symbols);
}

Val MakePair(Mem *mem, Val head, Val tail)
{
  Val pair = PairVal(Free(mem));

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

Val ListFrom(Mem *mem, Val list, u32 index)
{
  if (IsNil(list)) return nil;
  if (index == 0) return list;
  return ListFrom(mem, Tail(mem, list), index - 1);
}

Val ReverseOnto(Mem *mem, Val list, Val tail)
{
  if (IsNil(list)) return tail;
  return ReverseOnto(mem, Tail(mem, list), MakePair(mem, Head(mem, list), tail));
}

Val ListAppend(Mem *mem, Val list, Val value)
{
  list = ReverseOnto(mem, list, nil);
  return ReverseOnto(mem, MakePair(mem, value, list), nil);
}

Val ListConcat(Mem *mem, Val list1, Val list2)
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
  Val tuple = ObjVal(Free(mem));
  VecPush(mem->values, TupleHeader(count));

  for (u32 i = 0; i < count; i++) {
    VecPush(mem->values, nil);
  }

  return tuple;
}

u32 TupleLength(Mem *mem, Val tuple)
{
  Assert(IsTuple(mem, tuple));
  u32 index = RawObj(tuple);
  return HeaderVal(mem->values[index]);
}

Val TupleAt(Mem *mem, Val tuple, u32 i)
{
  Assert(IsTuple(mem, tuple));
  u32 index = RawObj(tuple);
  return mem->values[index + i + 1];
}

void TupleSet(Mem *mem, Val tuple, u32 i, Val val)
{
  Assert(IsTuple(mem, tuple));

  u32 index = RawObj(tuple);
  mem->values[index + i + 1] = val;
}

bool IsDict(Mem *mem, Val dict)
{
  u32 index = RawObj(dict);
  return HeaderType(mem->values[index]) == dictMask;
}

Val MakeDict(Mem *mem)
{
  Val keys = MakeTuple(mem, 0);
  Val vals = MakeTuple(mem, 0);
  return DictFrom(mem, keys, vals);
}

Val DictFrom(Mem *mem, Val keys, Val vals)
{
  Assert(IsTuple(mem, keys));
  Assert(IsTuple(mem, vals));

  u32 size = TupleLength(mem, keys);
  Val dict = ObjVal(Free(mem));
  VecPush(mem->values, DictHeader(size));
  VecPush(mem->values, keys);
  VecPush(mem->values, vals);
  return dict;
}

u32 DictSize(Mem *mem, Val dict)
{
  Assert(IsDict(mem, dict));

  u32 index = RawObj(dict);
  return HeaderVal(mem->values[index]);
}

Val DictKeys(Mem *mem, Val dict)
{
  Assert(IsDict(mem, dict));

  u32 index = RawObj(dict);
  return mem->values[index+1];
}

Val DictValues(Mem *mem, Val dict)
{
  Assert(IsDict(mem, dict));

  u32 index = RawObj(dict);
  return mem->values[index+2];
}

bool InDict(Mem *mem, Val dict, Val key)
{
  Assert(IsDict(mem, dict));

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
  Assert(IsDict(mem, dict));

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
  Assert(IsDict(mem, dict));

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
        return DictFrom(mem, keys, new_vals);
      }
      TupleSet(mem, new_vals, i, TupleAt(mem, vals, i));
    }
    Assert(false);
  } else {
    Val new_keys = MakeTuple(mem, count+1);
    Val new_vals = MakeTuple(mem, count+1);
    for (u32 i = 0; i < count; i++) {
      TupleSet(mem, new_keys, i, TupleAt(mem, keys, i));
      TupleSet(mem, new_vals, i, TupleAt(mem, vals, i));
    }
    TupleSet(mem, new_keys, count, key);
    TupleSet(mem, new_vals, count, value);
    return DictFrom(mem, new_keys, new_vals);
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
  Assert(IsSym(symbol));
  return MapGet(&mem->symbols, symbol.as_v);
}

bool IsBinary(Mem *mem, Val binary)
{
  u32 index = RawObj(binary);
  return HeaderType(mem->values[index]) == binaryMask;
}

Val MakeBinaryFrom(Mem *mem, char *str, u32 length)
{
  Val binary = ObjVal(Free(mem));
  VecPush(mem->values, BinaryHeader(length));

  u32 num_words = (length - 1) / sizeof(Val) + 1;
  GrowVec(mem->values, num_words);

  u8 *bytes = BinaryData(mem, binary);
  for (u32 i = 0; i < length; i++) {
    bytes[i] = str[i];
  }

  return binary;
}

Val MakeBinary(Mem *mem, char *str)
{
  return MakeBinaryFrom(mem, str, StrLen(str));
}

u32 BinaryLength(Mem *mem, Val binary)
{
  Assert(IsBinary(mem, binary));
  u32 index = RawObj(binary);
  return HeaderVal(mem->values[index]);
}

u8 *BinaryData(Mem *mem, Val binary)
{
  Assert(IsBinary(mem, binary));
  u32 index = RawObj(binary);
  return (u8*)(mem->values + index + 1);
}

bool IsTrue(Val value)
{
  return !(IsNil(value) || Eq(value, SymbolFor("false")));
}

static void PrintTail(Mem *mem, Val tail, u32 length)
{
  PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    Print("]");
  } else if (!IsPair(Tail(mem, tail))) {
    Print(" | ");
    PrintVal(mem, Tail(mem, tail));
    Print("]");
  } else {
    Print(" ");
    PrintTail(mem, Tail(mem, tail), length + 1);
  }
}

void PrintVal(Mem *mem, Val value)
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
    if (IsTagged(mem, value, SymbolFor("__procedure"))) {
      Print("[λ");
      Val params = ListAt(mem, value, 1);
      while (!IsNil(params)) {
        Print(" ");
        Print(SymbolName(mem, Head(mem, params)));
        params = Tail(mem, params);
      }
      Print("]");
    } else {
      Print("[");
      PrintTail(mem, value, 1);
    }
  } else if (IsTuple(mem, value)) {
    Print("#[");
    for (u32 i = 0; i < TupleLength(mem, value); i++) {
      PrintVal(mem, TupleAt(mem, value, i));
      if (i != TupleLength(mem, value) - 1) {
        Print(", ");
      }
    }
    Print("]");
  } else if (IsDict(mem, value)) {
    Print("{");
    Val keys = DictKeys(mem, value);
    Val vals = DictValues(mem, value);
    for (u32 i = 0; i < DictSize(mem, value); i++) {
      Val key = TupleAt(mem, keys, i);
      if (IsSym(key)) {
        Print(SymbolName(mem, key));
        Print(": ");
      } else if (!IsNil(key)) {
        PrintVal(mem, key);
        Print(" => ");
      }
      PrintVal(mem, TupleAt(mem, vals, i));
      if (i != DictSize(mem, value) - 1) {
        Print(", ");
      }
    }
    Print("}");
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
  } else {
    Print("<v");
    PrintIntN(RawObj(value), 4);
    Print(">");
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
  for (u32 i = 0; i < Free(mem); i++) {
    Print("[");
    PrintIntN(i, 4);
    Print("] ");
    DebugVal(mem, mem->values[i]);
    Print("\n");
  }
}

#undef Free
