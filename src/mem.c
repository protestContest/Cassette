#include "mem.h"
#include <univ.h>

static val *mem = 0;

void InitMem(void)
{
  if (mem) return;
  VecPush(mem, 0);
  VecPush(mem, 0);
  SetSymbolSize(valBits);
}

i32 MemAlloc(i32 count)
{
  i32 index;
  if (!mem) InitMem();
  index = VecCount(mem);
  GrowVec(mem, Max(2, count));
  return index;
}

i32 MemSize(void)
{
  return VecCount(mem);
}

val MemGet(i32 index)
{
  if (index < 0 || index >= (i32)VecCount(mem)) return 0;
  return mem[index];
}

void MemSet(i32 index, val value)
{
  if (index < 0 || index >= (i32)VecCount(mem)) return;
  mem[index] = value;
}

val Pair(val head, val tail)
{
  i32 index = MemAlloc(2);
  MemSet(index, head);
  MemSet(index+1, tail);
  return PairVal(index);
}

val Head(val pair)
{
  return MemGet(RawVal(pair));
}

val Tail(val pair)
{
  return MemGet(RawVal(pair)+1);
}

i32 ListLength(val list)
{
  i32 count = 0;
  while (list && ValType(list) == pairType) {
    count++;
    list = Tail(list);
  }
  return count;
}

val ListGet(val list, i32 index)
{
  i32 i;
  for (i = 0; i < index; i++) list = Tail(list);
  return Head(list);
}

val ReverseList(val list)
{
  val reversed = 0;
  while (list && ValType(list) == pairType) {
    reversed = Pair(Head(list), reversed);
    list = Tail(list);
  }
  return reversed;
}

val Tuple(i32 length)
{
  i32 index = MemAlloc(length+1);
  MemSet(index, TupleHeader(length));
  return ObjVal(index);
}

val Binary(i32 length)
{
  i32 index = MemAlloc(Align(length, 4) / 4 + 1);
  MemSet(index, BinHeader(length));
  return ObjVal(index);
}

i32 ObjLength(val tuple)
{
  return RawVal(MemGet(RawVal(tuple)));
}

val ObjGet(val tuple, i32 index)
{
  if (index < 0 || index >= ObjLength(tuple)) return 0;
  return MemGet(RawVal(tuple)+index+1);
}

void ObjSet(val tuple, i32 index, val value)
{
  if (index < 0 || index >= ObjLength(tuple)) return;
  MemSet(RawVal(tuple)+index+1, value);
}
