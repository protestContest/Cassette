#pragma once
#include "vm.h"

Val ListToBin(VM *vm, Val args);
Val ListToTuple(VM *vm, Val args);
Val ListReverse(VM *vm, Val args);
Val ListTrunc(VM *vm, Val args);
Val ListTail(VM *vm, Val args);
Val ListJoin(VM *vm, Val args);