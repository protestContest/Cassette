#pragma once
#include "vm.h"

Val BitAnd(VM *vm, Val args);
Val BitOr(VM *vm, Val args);
Val BitNot(VM *vm, Val args);
Val BitXOr(VM *vm, Val args);
Val BitShift(VM *vm, Val args);
Val BitCount(VM *vm, Val args);
