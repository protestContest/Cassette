#pragma once
#include "runtime/vm.h"

/*
 * Primitive functions for use by the VM.
 *
 * A primitive function must return a value to be pushed onto the stack. If an error occurs, it can
 * be signaled with RuntimeError(vm). When called, function arguments will be on the stack in
 * reverse order.
 */

typedef u32 (*PrimFn)(VM *vm);

typedef struct {
  char *name;
  PrimFn fn;
} PrimDef;

i32 PrimitiveID(u32 name);
char *PrimitiveName(u32 id);
PrimFn PrimitiveFn(u32 id);
