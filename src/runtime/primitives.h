#pragma once
#include "vm.h"

#define KernelMod   0x7FDE6DA7 /* Kernel */

typedef Result (*PrimitiveFn)(u32 num_args, VM *vm);

typedef struct {
  Val name;
  PrimitiveFn fn;
} PrimitiveDef;

typedef struct {
  Val module;
  u32 num_fns;
  PrimitiveDef *fns;
} PrimitiveModuleDef;

PrimitiveModuleDef *GetPrimitives(void);
u32 NumPrimitives(void);
Result DoPrimitive(Val mod, Val id, u32 num_args, VM *vm);
