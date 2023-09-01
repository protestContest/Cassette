#pragma once
#include "vm.h"

Val StdHead(VM *vm, Val args);
Val StdTail(VM *vm, Val args);
Val StdIsNil(VM *vm, Val args);
Val StdIsNum(VM *vm, Val args);
Val StdIsInt(VM *vm, Val args);
Val StdIsSym(VM *vm, Val args);
Val StdIsPair(VM *vm, Val args);
Val StdIsTuple(VM *vm, Val args);
Val StdIsBinary(VM *vm, Val args);
Val StdIsMap(VM *vm, Val args);
