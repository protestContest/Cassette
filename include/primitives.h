#pragma once
#include "vm.h"

/* Primitive functions for use by the VM. */

typedef u32 (*PrimFn)(struct VM *vm);

i32 PrimitiveID(u32 name);
PrimFn GetPrimitive(u32 id);
char *PrimitiveName(u32 id);
