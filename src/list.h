#pragma once
#include "vm.h"

Val Reverse(VM *vm, Val list);
Val ReverseOnto(VM *vm, Val list, Val tail);
Val ListAt(VM *vm, Val list, u32 n);
Val MakeList(VM *vm, u32 length, ...);

Val MakeLoop(VM *vm);
bool IsLoop(VM *vm, Val list);
Val LoopAppend(VM *vm, Val list, Val value);
Val CloseLoop(VM *vm, Val list);
