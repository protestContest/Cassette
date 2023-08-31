#pragma once
#include "vm.h"

Val BinToList(VM *vm, Val args);
Val BinTrunc(VM *vm, Val args);
Val BinAfter(VM *vm, Val args);
Val BinSlice(VM *vm, Val args);
Val BinJoin(VM *vm, Val args);
