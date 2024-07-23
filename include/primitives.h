#pragma once
#include "vm.h"

/* Primitive functions for use by the VM. */

typedef struct {
  char *name;
  PrimFn fn;
  char *type;
} PrimDef;

PrimDef *Primitives(void);
u32 NumPrimitives(void);
