#include "list.h"
#include "vm.h"

Value ListAt(Value list, Value n)
{
  if (RawVal(n) == 0) {
    return Head(list);
  }

  return ListAt(Tail(list), Decr(n));
}

u32 ListLength(Value list)
{
  if (IsNil(list)) return 0;
  return ListLength(Tail(list));
}

Value ReverseListOnto(Value value, Value tail)
{
  if (IsNil(value)) return tail;

  Value next = Tail(value);
  SetTail(value, tail);
  return ReverseListOnto(next, value);
}

Value ReverseList(Value list)
{
  return ReverseListOnto(list, nil_val);
}

void ListPush(Value stack, Value value)
{
  Value head = MakePair(value, Head(stack));
  SetHead(stack, head);
}

void ListAppendItem(Value list1, Value list2)
{
  SetTail(list1, list2);
}
