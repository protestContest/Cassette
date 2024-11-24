#pragma once
#include "runtime/vm.h"

/* Primitive functions for use by the VM. */

typedef u32 (*PrimFn)(VM *vm);

typedef struct {
  char *name;
  PrimFn fn;
} PrimDef;

i32 PrimitiveID(u32 name);
char *PrimitiveName(u32 id);
PrimFn PrimitiveFn(u32 id);
