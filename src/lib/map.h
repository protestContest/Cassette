#pragma once
#include "vm.h"

Val StdMapNew(VM *vm, Val args);
Val StdMapKeys(VM *vm, Val args);
Val StdMapValues(VM *vm, Val args);
Val StdMapPut(VM *vm, Val args);
Val StdMapGet(VM *vm, Val args);
Val StdMapDelete(VM *vm, Val args);
Val StdMapToList(VM *vm, Val args);
