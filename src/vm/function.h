#pragma once
#include "heap.h"
#include "vm.h"

typedef Val (*PrimitiveImpl)(VM *vm, Val args);

typedef struct {
  char *module;
  char *name;
  PrimitiveImpl impl;
} PrimitiveDef;

bool IsFunc(Val func, Heap *mem);
Val MakeFunc(Val entry, Val env, Heap *mem);
Val FuncEntry(Val func, Heap *mem);
Val FuncEnv(Val func, Heap *mem);

void SetPrimitives(PrimitiveDef *prims);
bool IsPrimitive(Val value, Heap *mem);
Val DoPrimitive(Val prim, VM *vm);
Val DefinePrimitives(Heap *mem);
