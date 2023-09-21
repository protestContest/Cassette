#pragma once
#include "value.h"
#include "symbols.h"

typedef struct Mem {
  u32 capacity;
  u32 count;
  Val **values;
  SymbolMap symbols;
} Mem;

void InitMem(Mem *mem, u32 size);
void DestroyMem(Mem *mem);
u32 MemSize(Mem *mem);
void PushVal(Mem *mem, Val value);
