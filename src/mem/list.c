#include "mem.h"
#include "univ/system.h"

Val Pair(Val head, Val tail, Mem *mem)
{
  Val pair;

  if (!CheckMem(mem, 2)) {
    PushRoot(mem, head);
    PushRoot(mem, tail);
    CollectGarbage(mem);
    tail = PopRoot(mem);
    head = PopRoot(mem);
  }
  Assert(CheckMem(mem, 2));

  pair = PairVal(MemNext(mem));
  PushMem(mem, head);
  PushMem(mem, tail);
  return pair;
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

Val ListCat(Val list1, Val list2, Mem *mem)
{
  PushRoot(mem, list2);
  list1 = ReverseList(list1, Nil, mem);
  list2 = PopRoot(mem);
  return ReverseList(list1, list2, mem);
}

Val ReverseList(Val list, Val tail, Mem *mem)
{
  while (list != Nil) {
    PushRoot(mem, Tail(list, mem));
    tail = Pair(Head(list, mem), tail, mem);
    list = PopRoot(mem);
  }
  return tail;
}
