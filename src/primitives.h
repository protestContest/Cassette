#pragma once
#include "mem.h"
#include "vm.h"

bool IsPrimitive(Val op, Mem *mem);
Val DoPrimitive(Val op, Val args, VM *vm);
void DefinePrimitives(Val env, Mem *mem);

// #include "eval.h"
// #include "mem.h"
// #include "vm.h"

