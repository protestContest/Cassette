#include "mem.h"
#include "env.h"
#include <stdarg.h>

#define NextFree(mem)   VecCount(mem->values)

void InitMem(Mem *mem, u32 size)
{
  if (size > 0) {
    mem->values = NewVec(Val, size);
  } else {
    mem->values = NULL;
  }
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);
  InitMap(&mem->symbols);
  mem->symbol_names = NULL;
}

void DestroyMem(Mem *mem)
{
  FreeVec(mem->values);
  FreeVec(mem->symbol_names);
  DestroyMap(&mem->symbols);
}

Val MakePair(Mem *mem, Val head, Val tail)
{
  Val pair = PairVal(NextFree(mem));

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
  if (!IsObj(tuple)) return false;
  u32 index = RawObj(tuple);
  return HeaderType(mem->values[index]) == tupleMask;
}

Val MakeTuple(Mem *mem, u32 count)
{
  Val tuple = ObjVal(NextFree(mem));
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
  if (!IsObj(dict)) return false;
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
  Val dict = ObjVal(NextFree(mem));
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
    u32 name_num = VecCount(mem->symbol_names);
    VecPush(mem->symbol_names, sym_str);
    for (u32 i = 0; i < length; i++) sym_str[i] = str[i];
    sym_str[length] = '\0';
    MapSet(&mem->symbols, sym.as_v, name_num);
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
  u32 index = MapGet(&mem->symbols, symbol.as_v);
  return mem->symbol_names[index];
}

bool IsBinary(Mem *mem, Val binary)
{
  if (!IsObj(binary)) return false;
  u32 index = RawObj(binary);
  return HeaderType(mem->values[index]) == binaryMask;
}

Val MakeBinaryFrom(Mem *mem, char *str, u32 length)
{
  Val binary = ObjVal(NextFree(mem));
  VecPush(mem->values, BinaryHeader(length));

  if (length == 0) {
    return binary;
  }

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

void BinaryToString(Mem *mem, Val binary, char *dst)
{
  Assert(IsBinary(mem, binary));

  u32 length = BinaryLength(mem, binary);
  for (u32 i = 0; i < length; i++) {
    dst[i] = BinaryData(mem, binary)[i];
  }
  dst[length] = '\0';
}

bool IsTrue(Val value)
{
  return !(IsNil(value) || Eq(value, SymbolFor("false")));
}

bool IOListToString(Mem *mem, Val list, Buf *buf)
{
  while (!IsNil(list)) {
    Val val = Head(mem, list);
    if (IsInt(val)) {
      u8 c = RawInt(val);
      AppendByte(buf, c);
    } else if (IsBinary(mem, val)) {
      Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
    } else if (IsList(mem, val)) {
      if (!IOListToString(mem, val, buf)) {
        return false;
      }
    } else {
      return false;
    }
    list = Tail(mem, list);
  }
  return true;
}

bool ValToString(Mem *mem, Val val, Buf *buf)
{
  if (IsNil(val)) {
    Append(buf, (u8*)"nil", 3);
  } else if (IsNum(val)) {
    AppendFloat(buf, RawNum(val));
  } else if (IsInt(val)) {
    AppendInt(buf, RawInt(val));
  } else if (IsSym(val)) {
    char *str = SymbolName(mem, val);
    Append(buf, (u8*)str, StrLen(str));
  } else if (IsBinary(mem, val)) {
    Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
  } else if (IsList(mem, val)) {
    return IOListToString(mem, val, buf);
  } else {
    return false;
  }

  return true;
}

static u32 PrintTail(Mem *mem, Val tail)
{
  u32 printed = 0;
  printed += PrintVal(mem, Head(mem, tail));
  if (IsNil(Tail(mem, tail))) {
    printed += Print("]");
  } else if (!IsPair(Tail(mem, tail))) {
    printed += Print(" | ");
    printed += PrintVal(mem, Tail(mem, tail));
    printed += Print("]");
  } else {
    printed += Print(" ");
    printed += PrintTail(mem, Tail(mem, tail));
  }
  return printed;
}

u32 PrintVal(Mem *mem, Val value)
{
  u32 printed = 0;
  if (IsNil(value)) {
    printed += Print("nil");
  } else if (IsNum(value)) {
    printed += PrintFloat(value.as_f);
  } else if (IsInt(value)) {
    printed += PrintInt(RawInt(value));
  } else if (IsSym(value)) {
    if (Eq(value, SymbolFor("true"))) {
      printed += Print("true");
    } else if (Eq(value, SymbolFor("false"))) {
      printed += Print("false");
    } else {
      printed += Print(":");
      char *str = SymbolName(mem, value);
      printed += Print(str);
    }
  } else if (IsPair(value)) {
    if (IsTagged(mem, value, SymbolFor("λ"))) {
      printed += Print("[λ ") - 1;
      Val body = ProcBody(value, mem);
      printed += PrintVal (mem, body);
      printed += Print("]");
    } else if (IsTagged(mem, value, SymbolFor("α"))) {
      printed += Print("[α ") - 1;
      printed += Print(SymbolName(mem, Tail(mem, value)));
      printed += Print("]");
    } else if (IsTagged(mem, Head(mem, value), SymbolFor("ε"))) {
      printed += Print("[ε") - 1;
      printed += PrintInt(ListLength(mem, value) - 1);
      printed += Print("]");
    } else {
      printed += Print("[");
      printed += PrintTail(mem, value);
    }
  } else if (IsTuple(mem, value)) {
    printed += Print("#[");
    for (u32 i = 0; i < TupleLength(mem, value); i++) {
      printed += PrintVal(mem, TupleAt(mem, value, i));
      if (i != TupleLength(mem, value) - 1) {
        printed += Print(", ");
      }
    }
    printed += Print("]");
  } else if (IsDict(mem, value)) {
    printed += Print("{");
    Val keys = DictKeys(mem, value);
    Val vals = DictValues(mem, value);
    for (u32 i = 0; i < DictSize(mem, value); i++) {
      Val key = TupleAt(mem, keys, i);
      if (IsSym(key)) {
        printed += Print(SymbolName(mem, key));
        printed += Print(": ");
      } else if (!IsNil(key)) {
        printed += PrintVal(mem, key);
        printed += Print(" => ");
      }
      printed += PrintVal(mem, TupleAt(mem, vals, i));
      if (i != DictSize(mem, value) - 1) {
        printed += Print(", ");
      }
    }
    printed += Print("}");
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
    printed += PrintIntN(RawObj(value), 4, ' ');
    printed += Print(">");
  }

  return printed;
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
    PrintIntN(RawObj(value), 4, ' ');
    Print(">");
  }
}

void PrintMem(Mem *mem)
{
  for (u32 i = 0; i < NextFree(mem); i++) {
    Print("[");
    PrintIntN(i, 4, ' ');
    Print("] ");
    DebugVal(mem, mem->values[i]);
    Print("\n");
  }
}
