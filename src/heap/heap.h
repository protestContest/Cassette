#pragma once
#include "value.h"
#include "univ/hashmap.h"

typedef struct {
  u32 capacity;
  u32 count;
  Val *values;
  u32 strings_capacity;
  u32 strings_count;
  char *strings;
  HashMap string_map;
} Heap;

void InitMem(Heap *mem, u32 size);
void DestroyMem(Heap *mem);
u32 MemSize(Heap *mem);
void CopyStrings(Heap *src, Heap *dst);
void PushVal(Heap *mem, Val value);
