#pragma once
#include "value.h"

typedef struct {
  Val *values;
  char *strings;
  HashMap string_map;
} Heap;

void InitMem(Heap *mem, u32 size);
void DestroyMem(Heap *mem);
u32 MemSize(Heap *mem);
void CopyStrings(Heap *src, Heap *dst);
