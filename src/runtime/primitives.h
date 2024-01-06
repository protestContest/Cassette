#pragma once
#include "vm.h"

typedef Result (*PrimitiveFn)(u32 num_args, VM *vm);

typedef struct {
  char *desc;
  Val name;
  PrimitiveFn fn;
} PrimitiveDef;

PrimitiveDef *GetPrimitives(void);
u32 NumPrimitives(void);
Result DoPrimitive(Val id, u32 num_args, VM *vm);
