#pragma once
#include "heap.h"
#include "vm.h"

typedef Val (*PrimitiveImpl)(VM *vm);

typedef struct {
  char *name;
  PrimitiveImpl impl;
} PrimitiveDef;


bool IsFunc(Val func, Heap *mem);
Val MakeFunc(Val entry, Val env, Heap *mem);
Val FuncEntry(Val func, Heap *mem);
Val FuncEnv(Val func, Heap *mem);

bool IsPrimitive(Val value, Heap *mem);
Val DoPrimitive(Val prim, VM *vm);

void SetPrimitives(PrimitiveDef *primitives, u32 count);
Val PrimitiveNames(Heap *mem);
Val GetPrimitives(Heap *mem);
