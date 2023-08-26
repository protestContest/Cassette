#pragma once
#include "vm.h"

PrimitiveDef *StdLib(void);

Val StdHead(VM *vm, Val args);
Val StdTail(VM *vm, Val args);
