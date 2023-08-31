#pragma once
#include "vm.h"

Val StringValid(VM *vm, Val args);
Val StringLength(VM *vm, Val args);
Val StringAt(VM *vm, Val args);
Val StringSlice(VM *vm, Val args);
