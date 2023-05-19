#include "mem.h"
#include "env.h"

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
  InitStringTable(&mem->symbols);
}

void DestroyMem(Mem *mem)
{
  FreeVec(mem->values);
  DestroyStringTable(&mem->symbols);
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

bool IsTagged(Mem *mem, Val list, char *tag)
{
  if (!IsPair(list)) return false;
  return Eq(Head(mem, list), SymbolFor(tag));
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

bool IsMap(Mem *mem, Val value)
{
  if (!IsObj(value)) return false;
  u32 index = RawObj(value);
  return HeaderType(mem->values[index]) == mapMask;
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

Val MakeBinary(Mem *mem, u32 length)
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
    bytes[i] = 0;
  }

  return binary;
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

static u32 IOListToString(Mem *mem, Val list, Buf *buf)
{
  u32 len = 0;
  while (!IsNil(list)) {
    Val val = Head(mem, list);
    if (IsInt(val)) {
      u8 c = RawInt(val);
      len += AppendByte(buf, c);
    } else if (IsSym(val)) {
      len += Append(buf, (u8*)SymbolName(mem, val), SymbolLength(mem, val));
    } else if (IsBinary(mem, val)) {
      len += Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
    } else if (IsList(mem, val)) {
      if (!IOListToString(mem, val, buf)) {
        return len;
      }
    } else {
      return len;
    }
    list = Tail(mem, list);
  }
  return len;
}

u32 ValToString(Mem *mem, Val val, Buf *buf)
{
  if (IsNil(val)) {
    return Append(buf, (u8*)"nil", 3);
  } else if (IsNum(val)) {
    return AppendFloat(buf, RawNum(val));
  } else if (IsInt(val)) {
    return AppendInt(buf, RawInt(val));
  } else if (IsSym(val)) {
    char *str = SymbolName(mem, val);
    return Append(buf, (u8*)str, SymbolLength(mem, val));
  } else if (IsBinary(mem, val)) {
    return Append(buf, BinaryData(mem, val), BinaryLength(mem, val));
  } else if (IsList(mem, val)) {
    return IOListToString(mem, val, buf);
  } else {
    return 0;
  }
}

u32 PrintValStr(Mem *mem, Val val)
{
  return ValToString(mem, val, output);
}

u32 ValStrLen(Mem *mem, Val val)
{
  if (IsSym(val)) {
    return SymbolLength(mem, val);
  } else if (IsBinary(mem, val)) {
    return BinaryLength(mem, val);
  } else {
    char str[255];
    Buf buf = MemBuf(str, 255);
    ValToString(mem, val, &buf);
    return buf.count;
  }
}
