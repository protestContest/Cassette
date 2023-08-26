#pragma once
#include "vm.h"

Val StdMapKeys(VM *vm, Val args);
Val StdMapValues(VM *vm, Val args);
Val StdMapPut(VM *vm, Val args);
Val StdMapDelete(VM *vm, Val args);
