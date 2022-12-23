#include "vm.h"

int main(void)
{
  VM vm;
  InitVM(&vm);

  StackPush(&vm, 1);
  StackPush(&vm, 2);
  MakePair(&vm, 'a', 'b');
  StackPush(&vm, 3);
  StackPush(&vm, 3);
  StackPush(&vm, 3);
  StackPush(&vm, 3);
  StackPush(&vm, 3);
  StackPush(&vm, 3);
  StackPop(&vm);
  MakeSymbol(&vm, "test");
  StackPush(&vm, 4);
  MakeTuple(&vm, 6, 41, 42, 43, 44, 45, 46);

  DumpVM(&vm);

  return 0;
}
