#include "symbols.h"
#include "mem.h"
#include "univ.h"

void InitSymbolMap(SymbolMap *symbols)
{
  symbols->capacity = 256;
  symbols->count = 0;
  symbols->names = (char **)NewHandle(sizeof(char*)*256);
  InitHashMap(&symbols->map);
}

void DestroySymbolMap(SymbolMap *symbols)
{
  DestroyHashMap(&symbols->map);
}

Val MakeSymbol(char *name, Mem *mem)
{
  return MakeSymbolFrom(name, StrLen(name), mem);
}

Val MakeSymbolFrom(char *name, u32 length, Mem *mem)
{
  SymbolMap *symbols = &mem->symbols;
  Val sym = HashBits(name, length, valBits);
  if (HashMapContains(&symbols->map, sym)) {
    return sym;
  } else {
    u32 bytes_needed = symbols->count + length + 1;
    u32 index;
    if (bytes_needed < symbols->capacity) {
      symbols->capacity = bytes_needed;
      SetHandleSize((Handle)symbols->names, symbols->capacity);
    }

    index = symbols->count;
    Copy(name, (*symbols->names) + index, length);
    (*symbols->names)[index + length] = '\0';
    HashMapSet(&symbols->map, sym, index);

    return sym;
  }
}

char *SymbolName(Val symbol, Mem *mem)
{
  SymbolMap *symbols = &mem->symbols;
  u32 index;
  if (!HashMapContains(&symbols->map, symbol)) return NULL;

  index = HashMapGet(&symbols->map, symbol);
  return *symbols->names + index;
}
