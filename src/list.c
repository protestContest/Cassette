#include "list.h"

Val ReverseOnto(VM *vm, Val list, Val tail)
{
  if (IsNil(list)) return tail;

  Val rest = Tail(vm, list);
  SetTail(vm, list, tail);
  return ReverseOnto(vm, rest, list);
}

Val Reverse(VM *vm, Val list)
{
  return ReverseOnto(vm, list, nil_val);
}

Val ListAt(VM *vm, Val list, u32 n)
{
  if (IsNil(list)) return nil_val;
  if (n == 0) return Head(vm, list);
  return ListAt(vm, Tail(vm, list), n - 1);
}

Val MakeList(VM *vm, u32 length, ...)
{
  va_list args;
  va_start(args, length);
  Val list = nil_val;
  for (u32 i = 0; i < length; i++) {
    Val arg = va_arg(args, Val);
    list = MakePair(vm, arg, list);
  }
  va_end(args);
  return Reverse(vm, list);
}

#define LoopMark(vm)   SymbolFor(vm, "_loop_", 6)

Val MakeLoop(VM *vm)
{
  Val list = MakePair(vm, LoopMark(vm), nil_val);
  SetTail(vm, list, list);
  return list;
}

bool IsLoop(VM *vm, Val list)
{
  return Eq(Head(vm, list), LoopMark(vm));
}

Val LoopAppend(VM *vm, Val list, Val value)
{
  Val item = MakePair(vm, value, Tail(vm, list));
  SetTail(vm, list, item);
  return item;
}

Val CloseLoop(VM *vm, Val list)
{
  Val first_item = Tail(vm, Tail(vm, list));
  SetTail(vm, list, nil_val);
  return first_item;
}
