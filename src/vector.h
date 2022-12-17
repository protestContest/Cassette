#pragma once
#include "value.h"

typedef struct Vector {
  u32 size;
  Value *data;
} Vector;

void InitVector(Vector *vec, u32 count);
void VectorGrow(Vector *vec, u32 size);
u32 VectorSize(Vector *vec);
void VectorSet(Vector *vec, u32 i, Value value);
Value VectorGet(Vector *vec, u32 i);
Value *VectorRef(Vector *vec, u32 i);
