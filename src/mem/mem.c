#include "mem.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/hash.h"

#ifdef DEBUG
#include <stdio.h>
#endif

Val FloatVal(float num)
{
  union {Val as_v; float as_f;} convert;
  convert.as_f = num;
  return convert.as_v;
}

float RawFloat(Val value)
{
  union {Val as_v; float as_f;} convert;
  convert.as_v = value;
  return convert.as_f;
}

void InitMem(Mem *mem, u32 capacity)
{
  mem->values = Alloc(sizeof(Val) * capacity);
  mem->count = 0;
  mem->capacity = capacity;
  Pair(Nil, Nil, mem);
}

void DestroyMem(Mem *mem)
{
  if (mem->values) Free(mem->values);
  mem->count = 0;
  mem->capacity = 0;
  mem->values = 0;
}

void PushMem(Mem *mem, Val value)
{
  mem->values[mem->count++] = value;
}

Val PopMem(Mem *mem)
{
  return mem->values[--mem->count];
}

Val TypeOf(Val value, Mem *mem)
{
  if (IsFloat(value)) return FloatType;
  if (IsInt(value) || IsBignum(value, mem)) return IntType;
  if (IsSym(value)) return SymType;
  if (IsPair(value)) return PairType;
  if (IsTuple(value, mem)) return TupleType;
  if (IsBinary(value, mem)) return BinaryType;
  return Nil;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  if (!CheckCapacity(mem, 2)) ResizeMem(mem, 2*mem->capacity);
  Assert(CheckCapacity(mem, 2));
  PushMem(mem, head);
  PushMem(mem, tail);
  return pair;
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return mem->values[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return mem->values[RawVal(pair)+1];
}

void SetHead(Val pair, Val value, Mem *mem)
{
  mem->values[RawVal(pair)] = value;
}

void SetTail(Val pair, Val value, Mem *mem)
{
  mem->values[RawVal(pair)+1] = value;
}

u32 ListLength(Val list, Mem *mem)
{
  u32 length = 0;
  while (IsPair(list) && list != Nil) {
    length++;
    list = Tail(list, mem);
  }
  return length;
}

bool ListContains(Val list, Val item, Mem *mem)
{
  while (IsPair(list) && list != Nil) {
    if (Head(list, mem) == item) return true;
    list = Tail(list, mem);
  }
  return false;
}

Val ReverseList(Val list, Val tail, Mem *mem)
{
  Assert(IsPair(list) && IsPair(tail));
  while (list != Nil) {
    tail = Pair(Head(list, mem), tail, mem);
    list = Tail(list, mem);
  }
  return tail;
}

Val ListGet(Val list, u32 index, Mem *mem)
{
  Assert(IsPair(list));
  while (index > 0) {
    list = Tail(list, mem);
    if (list == Nil || !IsPair(list)) return Nil;
    index--;
  }
  return Head(list, mem);
}

Val ListCat(Val list1, Val list2, Mem *mem)
{
  Assert(IsPair(list1) && IsPair(list2));
  list1 = ReverseList(list1, Nil, mem);
  return ReverseList(list1, list2, mem);
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  if (!CheckCapacity(mem, length+1)) ResizeMem(mem, Max(2*mem->capacity, mem->count + length + 1));
  Assert(CheckCapacity(mem, length + 1));
  PushMem(mem, TupleHeader(length));
  for (i = 0; i < length; i++) {
    PushMem(mem, Nil);
  }
  return tuple;
}

bool IsTuple(Val value, Mem *mem)
{
  return IsObj(value) && IsTupleHeader(mem->values[RawVal(value)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal(mem->values[RawVal(tuple)]);
}

bool TupleContains(Val tuple, Val item, Mem *mem)
{
  u32 i;
  Assert(IsTuple(tuple, mem));
  for (i = 0; i < TupleLength(tuple, mem); i++) {
    if (TupleGet(tuple, i, mem) == item) return true;
  }

  return false;
}

void TupleSet(Val tuple, u32 i, Val value, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  mem->values[RawVal(tuple) + i + 1] = value;
}

Val TupleGet(Val tuple, u32 i, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  return mem->values[RawVal(tuple) + i + 1];
}

Val TupleCat(Val tuple1, Val tuple2, Mem *mem)
{
  Val result;
  u32 len1, len2, i;
  Assert(IsTuple(tuple1, mem) && IsTuple(tuple2, mem));
  len1 = TupleLength(tuple1, mem);
  len2 = TupleLength(tuple2, mem);
  result = MakeTuple(len1+len2, mem);
  for (i = 0; i < len1; i++) {
    TupleSet(result, i, TupleGet(tuple1, i, mem), mem);
  }
  for (i = 0; i < len2; i++) {
    TupleSet(result, len1+i, TupleGet(tuple1, i, mem), mem);
  }
  return result;
}

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = (size == 0) ? 1 : NumBinCells(size);
  u32 i;

  if (!CheckCapacity(mem, cells+1)) ResizeMem(mem, Max(2*mem->capacity, mem->count+cells+1));
  Assert(CheckCapacity(mem, cells + 1));
  PushMem(mem, BinaryHeader(size));

  for (i = 0; i < cells; i++) {
    PushMem(mem, 0);
  }
  return binary;
}

Val BinaryFrom(char *str, u32 size, Mem *mem)
{
  Val bin = MakeBinary(size, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, size);
  return bin;
}

bool IsBinary(Val value, Mem *mem)
{
  return IsObj(value) && IsBinaryHeader(mem->values[RawVal(value)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return RawVal(mem->values[RawVal(binary)]);
}

bool BinaryContains(Val binary, Val item, Mem *mem)
{
  u8 *bin_data, *item_data;
  Assert(IsBinary(binary, mem));
  bin_data = BinaryData(binary, mem);
  item_data = BinaryData(item, mem);

  if (IsInt(item)) {
    u32 i;
    u8 byte;
    if (RawInt(item) < 0 || RawInt(item) > 255) return false;
    byte = RawInt(item);
    for (i = 0; i < BinaryLength(binary, mem); i++) {
      if (bin_data[i] == byte) return true;
    }
  } else if (IsBinary(item, mem)) {
    u32 i, j, item_hash, test_hash;
    u32 item_length = BinaryLength(item, mem);
    u32 bin_length = BinaryLength(binary, mem);
    if (item_length > bin_length) return false;
    if (item_length == 0 || bin_length == 0) return false;

    item_hash = Hash(item_data, item_length);
    test_hash = Hash(bin_data, item_length);

    for (i = 0; i < bin_length - item_length; i++) {
      if (test_hash == item_hash) break;
      test_hash = SkipHash(test_hash, bin_data[i], item_length);
      test_hash = AppendHash(test_hash, bin_data[i+item_length]);
    }

    if (test_hash == item_hash) {
      for (j = 0; j < item_length; j++) {
        if (bin_data[i+j] != item_data[j]) return false;
      }
      return true;
    }
  }

  return false;
}

void *BinaryData(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return &mem->values[RawVal(binary)+1];
}

Val BinaryCat(Val binary1, Val binary2, Mem *mem)
{
  Val result;
  u32 len1, len2;
  u8 *data;
  Assert(IsBinary(binary1, mem) && IsBinary(binary2, mem));
  len1 = BinaryLength(binary1, mem);
  len2 = BinaryLength(binary2, mem);
  result = MakeBinary(len1+len2, mem);
  data = BinaryData(result, mem);
  Copy(BinaryData(binary1, mem), data, len1);
  Copy(BinaryData(binary2, mem), data+len1, len2);
  return result;
}

Val MakeBignum(i64 num, Mem *mem)
{
  Val value = ObjVal(mem->count);
  PushMem(mem, BignumHeader(2));
  PushMem(mem, 0);
  PushMem(mem, 0);

  ((i64*)(mem->values + RawVal(value) + 1))[0] = num;

  return value;
}

bool IsBignum(Val value, Mem *mem)
{
  return IsObj(value) && IsBignumHeader(mem->values[RawVal(value)]);
}

bool BignumGreater(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return a_val > b_val;
}

bool BignumEq(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return a_val == b_val;
}

Val BignumAdd(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return MakeBignum(a_val + b_val, mem);
}

Val BignumSub(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return MakeBignum(a_val - b_val, mem);
}

Val BignumMul(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return MakeBignum(a_val * b_val, mem);
}

Val BignumDiv(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return MakeBignum(a_val / b_val, mem);
}

Val BignumRem(Val a, Val b, Mem *mem)
{
  i64 a_val = ((i64*)(mem->values + RawVal(a) + 1))[0];
  i64 b_val = ((i64*)(mem->values + RawVal(b) + 1))[0];
  return MakeBignum(a_val % b_val, mem);
}

float NumToFloat(Val num, Mem *mem)
{
  if (IsFloat(num)) return RawFloat(num);
  if (IsInt(num)) return (float)RawInt(num);
  if (IsBignum(num, mem)) {
    i64 n = ((i64*)(mem->values + RawVal(num) + 1))[0];
    return (float)n;
  }
  return 0;
}

Val AddVal(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return FloatVal(fa+fb);
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumAdd(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    if (AddOverflows(ia, ib) || AddUnderflows(ia, ib)) {
       Val a = MakeBignum(ia, mem);
       Val b = MakeBignum(ib, mem);
      return BignumAdd(a, b, mem);
    } else {
      return IntVal(ia+ib);
    }
  }
}

Val SubVal(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return FloatVal(fa-fb);
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumSub(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    if (AddOverflows(ia, -ib) || AddUnderflows(ia, -ib)) {
       Val a = MakeBignum(ia, mem);
       Val b = MakeBignum(ib, mem);
      return BignumSub(a, b, mem);
    } else {
      return IntVal(ia-ib);
    }
  }
}

Val MultiplyVal(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return FloatVal(fa*fb);
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumMul(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    if (MulOverflows(ia, ib) || MulUnderflows(ia, ib)) {
       Val a = MakeBignum(ia, mem);
       Val b = MakeBignum(ib, mem);
      return BignumMul(a, b, mem);
    } else {
      return IntVal(ia*ib);
    }
  }
}

Val DivideVal(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return FloatVal(fa/fb);
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumDiv(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    return IntVal(ia/ib);
  }
}

Val RemainderVal(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  Assert(!IsFloat(a) && !IsFloat(b));

  if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumRem(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    return IntVal(ia%ib);
  }
}

bool ValLessThan(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return fa < fb;
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return !BignumGreater(ba, bb, mem) && !BignumEq(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    return ia < ib;
  }
}

bool ValGreaterThan(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return fa > fb;
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumGreater(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    return ia > ib;
  }
}

bool NumValEqual(Val a, Val b, Mem *mem)
{
  Assert(IsNum(a, mem) && IsNum(b, mem));
  if (IsFloat(a) || IsFloat(b)) {
    float fa = NumToFloat(a, mem);
    float fb = NumToFloat(b, mem);
    return fa == fb;
  } else if (IsBignum(a, mem) || IsBignum(b, mem)) {
    Val ba = a;
    Val bb = b;
    if (IsInt(ba)) ba = MakeBignum(RawInt(a), mem);
    if (IsInt(bb)) bb = MakeBignum(RawInt(b), mem);
    return BignumEq(ba, bb, mem);
  } else {
    i32 ia = RawInt(a);
    i32 ib = RawInt(b);
    return ia == ib;
  }
}

bool CheckCapacity(Mem *mem, u32 amount)
{
  return mem->count + amount <= mem->capacity;
}

void ResizeMem(Mem *mem, u32 capacity)
{
  mem->capacity = capacity;
  mem->values = Realloc(mem->values, sizeof(Val)*capacity);
}

static Val ValSize(Val value)
{
  if (IsTupleHeader(value)) {
    return RawVal(value) + 1;
  } else if (IsBinaryHeader(value)) {
    return NumBinCells(RawVal(value)) + 1;
  } else if (IsBignumHeader(value)) {
    return RawVal(value) + 1;
  } else {
    return 1;
  }
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val = Nil;

  if (value == Nil) return Nil;
  if (!IsObj(value) && !IsPair(value)) return value;

  if (from->values[RawVal(value)] == Moved) {
    return from->values[RawVal(value)+1];
  }

  if (IsPair(value)) {
    new_val = Pair(Head(value, from), Tail(value, from), to);
  } else if (IsObj(value)) {
    u32 i;
    new_val = ObjVal(to->count);
    for (i = 0; i < ValSize(from->values[RawVal(value)]); i++) {
      PushMem(to, from->values[RawVal(value)+i]);
    }
  }

  from->values[RawVal(value)] = Moved;
  from->values[RawVal(value)+1] = new_val;
  return new_val;
}

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem)
{
  u32 i;
  Mem new_mem;

#ifdef DEBUG
  printf("GARBAGE DAY!!!\n");
#endif

  InitMem(&new_mem, mem->capacity);

  for (i = 0; i < num_roots; i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  i = 2;
  while (i < new_mem.count) {
    Val value = new_mem.values[i];
    if (IsBinaryHeader(value) || IsBignumHeader(value)) {
      i += ValSize(value);
    } else {
      new_mem.values[i] = CopyValue(new_mem.values[i], mem, &new_mem);
      i++;
    }
  }

  Free(mem->values);
  *mem = new_mem;
}
