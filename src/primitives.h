#pragma once
#include "vm.h"

typedef Val (*PrimitiveFn)(u32 num_args, VM *vm);

typedef struct {
  char *name;
  PrimitiveFn fn;
} PrimitiveDef;

PrimitiveDef *Primitives(void);
u32 NumPrimitives(void);
Val DefinePrimitives(Mem *mem);
Val DoPrimitive(Val id, u32 num_args, VM *vm);
