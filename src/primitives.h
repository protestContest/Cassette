#pragma once
#include "mem.h"
#include "vm.h"

typedef Val (*PrimitiveFn)(u32 num_args, VM *vm);

PrimitiveFn GetPrimitive(u32 index);
char *PrimitiveName(u32 index);
u32 NumPrimitives(void);
void DefinePrimitives(Val env, Mem *mem);
Val ApplyPrimitive(Val func, u32 num_args, VM *vm);
