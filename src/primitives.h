#pragma once
#include "mem.h"
#include "vm.h"

typedef Val (*PrimitiveFn)(VM *vm);

PrimitiveFn GetPrimitive(u32 index);
void DefinePrimitives(Val env, Mem *mem);
