#include "list.h"
#include "mem.h"

Value ReverseWith(Value value, Value end);

Value Reverse(Value list)
{
  return ReverseWith(list, nilVal);
}

Value ReverseWith(Value value, Value end)
{
  if (IsNil(value)) return end;

  Value next = Tail(value);
  SetTail(value, end);
  return ReverseWith(next, value);
}
