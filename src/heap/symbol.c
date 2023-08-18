#include "symbol.h"

Val MakeSymbol(char *name, Heap *mem)
{
  return MakeSymbolFrom(name, StrLen(name), mem);
}

Val MakeSymbolFrom(char *name, u32 length, Heap *mem)
{
  Val sym = SymbolFrom(name, length);
  if (HashMapContains(&mem->string_map, sym.as_i)) {
    return sym;
  }

  u32 index = VecCount(mem->strings);
  GrowVec(mem->strings, length);
  Copy(name, mem->strings + index, length);
  VecPush(mem->strings, '\0');
  HashMapSet(&mem->string_map, sym.as_i, index);

  return sym;
}

char *SymbolName(Val symbol, Heap *mem)
{
  if (!HashMapContains(&mem->string_map, symbol.as_i)) return NULL;

  u32 index = HashMapGet(&mem->string_map, symbol.as_i);
  return mem->strings + index;
}
