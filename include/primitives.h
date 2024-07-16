#pragma once
#include "vm.h"

typedef struct {
  char *name;
  PrimFn fn;
  char *type;
} PrimDef;

PrimDef *Primitives(void);
u32 NumPrimitives(void);
