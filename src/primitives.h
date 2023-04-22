#pragma once
#include "eval.h"
#include "mem.h"
#include "vm.h"

Val DoPrimitive(Val op, Val args, VM *vm);
void DefinePrimitives(Val env, Mem *mem);
