#include "mem.h"
#include <univ.h>

static i32 *mem = 0;

i32 MemAlloc(i32 count)
{
  i32 index;
  if (!mem) {
    VecPush(mem, 0);
    VecPush(mem, 0);
    SetSymbolSize(valBits);
  }
  index = VecCount(mem);
  GrowVec(mem, count);
  return index;
}

i32 MemSize(void)
{
  return VecCount(mem);
}

i32 MemGet(i32 index)
{
  if (index < 0 || index >= (i32)VecCount(mem)) return 0;
  return mem[index];
}

void MemSet(i32 index, i32 value)
{
  if (index < 0 || index >= (i32)VecCount(mem)) return;
  mem[index] = value;
}

i32 Pair(i32 head, i32 tail)
{
  i32 index = MemAlloc(2);
  MemSet(index, head);
  MemSet(index+1, tail);
  return PairVal(index);
}

i32 Head(i32 pair)
{
  return MemGet(RawVal(pair));
}

i32 Tail(i32 pair)
{
  return MemGet(RawVal(pair)+1);
}

i32 ListLength(i32 list)
{
  i32 count = 0;
  while (list && ValType(list) == pairType) {
    count++;
    list = Tail(list);
  }
  return count;
}

i32 ListGet(i32 list, i32 index)
{
  i32 i;
  for (i = 0; i < index; i++) list = Tail(list);
  return Head(list);
}

i32 ReverseList(i32 list)
{
  i32 reversed = 0;
  while (list && ValType(list) == pairType) {
    reversed = Pair(Head(list), reversed);
    list = Tail(list);
  }
  return reversed;
}

i32 Tuple(i32 length)
{
  i32 index = MemAlloc(length+1);
  MemSet(index, TupleHeader(length));
  return ObjVal(index);
}

i32 Binary(i32 length)
{
  i32 index = MemAlloc(Align(length, 4) / 4 + 1);
  MemSet(index, BinHeader(length));
  return ObjVal(index);
}

i32 ObjLength(i32 tuple)
{
  return RawVal(MemGet(RawVal(tuple)));
}

i32 ObjGet(i32 tuple, i32 index)
{
  if (index < 0 || index >= ObjLength(tuple)) return 0;
  return MemGet(RawVal(tuple)+index+1);
}

void ObjSet(i32 tuple, i32 index, i32 value)
{
  if (index < 0 || index >= ObjLength(tuple)) return;
  MemSet(RawVal(tuple)+index+1, value);
}
