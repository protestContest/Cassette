#include "mem.h"
#include "univ/system.h"

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair;
  if (mem->count + 2 > mem->capacity) {
    PushRoot(mem, head);
    PushRoot(mem, tail);
    CollectGarbage(mem);
    tail = PopRoot(mem, tail);
    head = PopRoot(mem, head);
  }
  if (mem->count + 2 > mem->capacity) ResizeMem(mem, 2*mem->capacity);
  Assert(mem->count + 2 <= mem->capacity);

  pair = PairVal(mem->count);
  PushMem(mem, head);
  PushMem(mem, tail);
  return pair;
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
  while (list != Nil) {
    tail = Pair(Head(list, mem), tail, mem);
    list = Tail(list, mem);
  }
  return tail;
}

Val ListGet(Val list, u32 index, Mem *mem)
{
  while (index > 0) {
    list = Tail(list, mem);
    if (list == Nil || !IsPair(list)) return Nil;
    index--;
  }
  return Head(list, mem);
}

Val ListCat(Val list1, Val list2, Mem *mem)
{
  list1 = ReverseList(list1, Nil, mem);
  return ReverseList(list1, list2, mem);
}
