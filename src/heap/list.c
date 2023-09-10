#include "list.h"
#include "tuple.h"

Val Pair(Val head, Val tail, Heap *mem)
{
  u32 index = MemSize(mem);
  VecPush(mem->values, head);
  VecPush(mem->values, tail);
  return PairVal(index);
}

Val Head(Val pair, Heap *mem)
{
  return mem->values[RawVal(pair)];
}

Val Tail(Val pair, Heap *mem)
{
  return mem->values[RawVal(pair)+1];
}

void SetHead(Val pair, Val head, Heap *mem)
{
  mem->values[RawVal(pair)] = head;
}

void SetTail(Val pair, Val tail, Heap *mem)
{
  mem->values[RawVal(pair)+1] = tail;
}

u32 ListLength(Val list, Heap *mem)
{
  u32 length = 0;
  while (!IsNil(list)) {
    length++;
    list = Tail(list, mem);
  }
  return length;
}

bool ListContains(Val list, Val value, Heap *mem)
{
  while (!IsNil(list)) {
    if (Eq(value, Head(list, mem))) return true;
    list = Tail(list, mem);
  }
  return false;
}

Val ListAt(Val list, u32 pos, Heap *mem)
{
  return Head(TailList(list, pos, mem), mem);
}

bool IsTaggedWith(Val list, Val tag, Heap *mem)
{
  if (IsPair(list)) {
    return Eq(tag, Head(list, mem));
  } else if (IsTuple(list, mem)) {
    return Eq(tag, TupleGet(list, 0, mem));
  } else {
    return false;
  }
}

bool IsTagged(Val list, char *tag, Heap *mem)
{
  if (IsPair(list)) {
    return Eq(SymbolFor(tag), Head(list, mem));
  } else if (IsTuple(list, mem)) {
    return Eq(SymbolFor(tag), TupleGet(list, 0, mem));
  } else {
    return false;
  }
}

Val TailList(Val list, u32 pos, Heap *mem)
{
  for (u32 i = 0; i < pos; i++) {
    list = Tail(list, mem);
  }
  return list;
}

Val TruncList(Val list, u32 pos, Heap *mem)
{
  if (pos == 0) return nil;
  return Pair(Head(list, mem), TruncList(Tail(list, mem), pos-1, mem), mem);
}

Val JoinLists(Val a, Val b, Heap *mem)
{
  if (IsNil(a)) return b;
  if (IsNil(b)) return a;

  return Pair(Head(a, mem), JoinLists(Tail(a, mem), b, mem), mem);
}

Val ListConcat(Val a, Val b, Heap *mem)
{
  if (IsNil(a)) return b;
  if (IsNil(b)) return a;

  while (!IsNil(Tail(a, mem))) a = Tail(a, mem);
  SetTail(a, b, mem);
  return a;
}

Val ReverseList(Val list, Heap *mem)
{
  Val new_list = nil;
  while (!IsNil(list)) {
    new_list = Pair(Head(list, mem), new_list, mem);
    list = Tail(list, mem);
  }
  return new_list;
}
