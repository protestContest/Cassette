#pragma once
#include "vm.h"

typedef Result (*PrimitiveFn)(u32 num_args, VM *vm);

typedef struct {
  char *name;
  PrimitiveFn fn;
} PrimitiveDef;

PrimitiveDef *Primitives(void);
u32 NumPrimitives(void);
Val DefinePrimitives(Mem *mem, SymbolTable *symbols);
Result DoPrimitive(Val id, u32 num_args, VM *vm);
Val CompileEnv(Mem *mem, SymbolTable *symbols);
