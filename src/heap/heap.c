#include "heap.h"
#include "symbol.h"
#include "univ/memory.h"
#include "univ/system.h"
#include "univ/math.h"

void InitMem(Heap *mem, u32 size)
{
  mem->capacity = size;
  mem->count = 0;
  mem->values = Allocate(size*sizeof(Val));
  PushVal(mem, nil);
  PushVal(mem, nil);

  mem->strings_capacity = 256;
  mem->strings_count = 0;
  mem->strings = Allocate(mem->strings_capacity);
  mem->string_map = EmptyHashMap;
  Assert(Eq(True, MakeSymbol("true", mem)));
  Assert(Eq(False, MakeSymbol("false", mem)));
}

void DestroyMem(Heap *mem)
{
  Free(mem->values);
  Free(mem->strings);
  DestroyHashMap(&mem->string_map);
}

u32 MemSize(Heap *mem)
{
  return mem->count;
}

void CopyStrings(Heap *src, Heap *dst)
{
  for (u32 i = 0; i < src->string_map.count; i++) {
    Val key = (Val){.as_i = HashMapKey(&src->string_map, i)};
    MakeSymbol(SymbolName(key, src), dst);
  }
}

void PushVal(Heap *mem, Val value)
{
  mem->capacity = Max(32, 2*mem->capacity);
  if (mem->count >= mem->capacity) {
    mem->values = Reallocate(mem->values, mem->capacity);
  }

  mem->values[mem->count++] = value;
}
