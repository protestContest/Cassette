#pragma once
#include "vm.h"

typedef Result (*PrimitiveFn)(u32 num_args, VM *vm);

typedef struct {
  u32 id;
  PrimitiveFn fn;
} PrimitiveDef;

Val PrimitiveEnv(Mem *mem);
Val CompileEnv(Mem *mem);
Result DoPrimitive(Val id, u32 num_args, VM *vm);
