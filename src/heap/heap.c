#include "heap.h"
#include "symbol.h"

void InitMem(Heap *mem, u32 size)
{
  mem->values = NewVec(Val, size);
  VecPush(mem->values, nil);
  VecPush(mem->values, nil);

  mem->strings = NewVec(char, 0);
  InitHashMap(&mem->string_map);
  MakeSymbol("true", mem);
  MakeSymbol("false", mem);
}

void DestroyMem(Heap *mem)
{
  FreeVec(mem->values);
  FreeVec(mem->strings);
  DestroyHashMap(&mem->string_map);
}

u32 MemSize(Heap *mem)
{
  return VecCount(mem->values);
}

void CopyStrings(Heap *src, Heap *dst)
{
  for (u32 i = 0; i < HashMapCount(&src->string_map); i++) {
    Val key = (Val){.as_i = GetHashMapKey(&src->string_map, i)};
    MakeSymbol(SymbolName(key, src), dst);
  }
}
