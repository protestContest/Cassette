#include "mem.h"
#include "symbols.h"
#include "univ.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

u32 PrintVal(Val value, SymbolTable *symbols)
{
  if (IsNil(value)) {
    return printf("nil");
  } else if (IsInt(value)) {
    return printf("%d", RawInt(value));
  } else if (IsFloat(value)) {
    return printf("%f", RawFloat(value));
  } else if (IsSym(value) && symbols) {
    return printf("%s", SymbolName(value, symbols));
  } else if (IsPair(value)) {
    return printf("p%d", RawVal(value));
  } else if (IsObj(value)) {
    return printf("t%d", RawVal(value));
  } else {
    return printf("%08X", value);
  }
}

void InitMem(Mem *mem, u32 capacity)
{
  mem->values = (Val**)NewHandle(sizeof(Val) * capacity);
  mem->count = 0;
  mem->capacity = capacity;
  Pair(Nil, Nil, mem);
}

void DestroyMem(Mem *mem)
{
  DisposeHandle((Handle)mem->values);
  mem->count = 0;
  mem->capacity = 0;
  mem->values = NULL;
}

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair = PairVal(mem->count);
  Assert(CheckCapacity(mem, 2));
  (*mem->values)[mem->count++] = head;
  (*mem->values)[mem->count++] = tail;
  return pair;
}

Val Head(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  Assert(IsPair(pair));
  return (*mem->values)[RawVal(pair)+1];
}

void SetHead(Val pair, Val value, Mem *mem)
{
  (*mem->values)[RawVal(pair)] = value;
}

void SetTail(Val pair, Val value, Mem *mem)
{
  (*mem->values)[RawVal(pair)+1] = value;
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

Val ReverseList(Val list, Mem *mem)
{
  Val reversed = Nil;
  Assert(IsPair(list));
  while (list != Nil) {
    reversed = Pair(Head(list, mem), reversed, mem);
    list = Tail(list, mem);
  }
  return reversed;
}

Val MakeTuple(u32 length, Mem *mem)
{
  u32 i;
  Val tuple = ObjVal(mem->count);
  Assert(CheckCapacity(mem, length + 1));
  (*mem->values)[mem->count++] = TupleHeader(length);
  for (i = 0; i < length; i++) {
    (*mem->values)[mem->count++] = Nil;
  }
  return tuple;
}

bool IsTuple(Val value, Mem *mem)
{
  return IsObj(value) && IsTupleHeader((*mem->values)[RawVal(value)]);
}

u32 TupleLength(Val tuple, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  return RawVal((*mem->values)[RawVal(tuple)]);
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
  (*mem->values)[RawVal(tuple) + i + 1] = value;
}

Val TupleGet(Val tuple, u32 i, Mem *mem)
{
  Assert(IsTuple(tuple, mem));
  Assert(i < TupleLength(tuple, mem));
  return (*mem->values)[RawVal(tuple) + i + 1];
}

Val MakeBinary(u32 size, Mem *mem)
{
  Val binary = ObjVal(mem->count);
  u32 cells = NumBinCells(size);
  u32 i;
  Assert(CheckCapacity(mem, cells + 1));
  (*mem->values)[mem->count++] = BinaryHeader(size);
  for (i = 0; i < cells; i++) {
    (*mem->values)[mem->count++] = 0;
  }
  return binary;
}

Val BinaryFrom(char *str, Mem *mem)
{
  u32 len = StrLen(str);
  Val bin = MakeBinary(len, mem);
  char *data = BinaryData(bin, mem);
  Copy(str, data, len);
  return bin;
}

bool IsBinary(Val value, Mem *mem)
{
  return IsObj(value) && IsBinaryHeader((*mem->values)[RawVal(value)]);
}

u32 BinaryLength(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return RawVal((*mem->values)[RawVal(binary)]);
}

void *BinaryData(Val binary, Mem *mem)
{
  Assert(IsBinary(binary, mem));
  return &(*mem->values)[RawVal(binary)];
}

bool CheckCapacity(Mem *mem, u32 amount)
{
  return mem->count + amount <= mem->capacity;
}

static Val CopyValue(Val value, Mem *from, Mem *to)
{
  Val new_val;

  if ((*from->values)[RawVal(value)] == Moved) {
    return (*from->values)[RawVal(value)+1];
  }

  if (IsPair(value)) {
    new_val = Pair(Head(value, from), Tail(value, from), to);
  } else if (IsTuple(value, from)) {
    u32 i;
    new_val = MakeTuple(TupleLength(value, from), to);
    for (i = 0; i < TupleLength(value, from); i++) {
      TupleSet(new_val, i, TupleGet(value, i, from), to);
    }
  } else if (IsBinary(value, from)) {
    new_val = MakeBinary(BinaryLength(value, from), to);
    Copy(BinaryData(value, from), BinaryData(new_val, to), BinaryLength(value, from));
  } else {
    return value;
  }

  SetHead(value, Moved, from);
  SetTail(value, new_val, from);
  return new_val;
}

void CollectGarbage(Val *roots, u32 num_roots, Mem *mem)
{
  u32 i;
  Mem new_mem;
  InitMem(&new_mem, mem->capacity);

  for (i = 0; i < num_roots; i++) {
    roots[i] = CopyValue(roots[i], mem, &new_mem);
  }

  i = 0;
  while (i < new_mem.count) {
    Val value = (*new_mem.values)[i];
    if (IsBinaryHeader(value)) {
      i += NumBinCells(RawVal(value));
    } else {
      (*new_mem.values)[i] = CopyValue((*new_mem.values)[i], mem, &new_mem);
      i++;
    }
  }

  DisposeHandle((Handle)mem->values);
  *mem = new_mem;
}
