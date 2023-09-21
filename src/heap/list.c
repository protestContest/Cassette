#include "list.h"
#include "univ.h"

Val Pair(Val head, Val tail, Mem *mem)
{
  u32 index = MemSize(mem);
  PushVal(mem, head);
  PushVal(mem, tail);
  return PairVal(index);
}

Val Head(Val pair, Mem *mem)
{
  return (*mem->values)[RawVal(pair)];
}

Val Tail(Val pair, Mem *mem)
{
  return (*mem->values)[RawVal(pair)+1];
}

void SetHead(Val pair, Val head, Mem *mem)
{
  (*mem->values)[RawVal(pair)] = head;
}

void SetTail(Val pair, Val tail, Mem *mem)
{
  (*mem->values)[RawVal(pair)+1] = tail;
}

u32 ListLength(Val list, Mem *mem)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(list, mem);
  }
  return length;
}

bool ListContains(Val list, Val value, Mem *mem)
{
  while (!IsNil(list)) {
    if (value == Head(list, mem)) return true;
    list = Tail(list, mem);
  }
  return false;
}

Val ListAt(Val list, u32 pos, Mem *mem)
{
  return Head(TailList(list, pos, mem), mem);
}

Val TailList(Val list, u32 pos, Mem *mem)
{
  u32 i;
  for (i = 0; i < pos; i++) {
    list = Tail(list, mem);
  }
  return list;
}

Val ListConcat(Val a, Val b, Mem *mem)
{
  if (IsNil(a)) return b;
  if (IsNil(b)) return a;

  while (!IsNil(Tail(a, mem))) a = Tail(a, mem);
  SetTail(a, b, mem);
  return a;
}

Val ReverseList(Val list, Mem *mem)
{
  Val new_list = Nil;
  while (!IsNil(list)) {
    new_list = Pair(Head(list, mem), new_list, mem);
    list = Tail(list, mem);
  }
  return new_list;
}
