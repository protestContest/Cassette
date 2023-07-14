#pragma once
#include "mem.h"
#include "vm.h"

typedef Val (*PrimitiveFn)(u32 num_args, VM *vm);

PrimitiveFn GetPrimitive(u32 index);
void DefinePrimitives(Val env, Mem *mem);
