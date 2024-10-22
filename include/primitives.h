#pragma once
#include "vm.h"

/* Primitive functions for use by the VM. */

typedef struct {
  char *name;
  PrimFn fn;
} PrimDef;

PrimDef *Primitives(void);
u32 NumPrimitives(void);
i32 PrimitiveID(u32 name);
u32 PrimitiveError(char *msg, VM *vm);
