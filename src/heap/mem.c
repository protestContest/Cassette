#include "mem.h"

void InitMem(Mem *mem, u32 size)
{
  mem->capacity = size;
  mem->count = 0;
  mem->values = (Val**)NewHandle(sizeof(Val)*size);
  PushVal(mem, Nil);
  PushVal(mem, Nil);
  InitSymbolMap(&mem->symbols);
}

void DestroyMem(Mem *mem)
{
  DisposeHandle((Handle)mem->values);
  DestroySymbolMap(&mem->symbols);
}

u32 MemSize(Mem *mem)
{
  return mem->count;
}

void PushVal(Mem *mem, Val value)
{
  if (mem->count >= mem->capacity) {
    mem->capacity = Max(32, 2*mem->capacity);
    SetHandleSize((Handle)mem->values, mem->capacity);
  }

  (*mem->values)[mem->count++] = value;
}
