#include "symbol.h"
#include "univ/hash.h"
#include "univ/hashmap.h"
#include "univ/vec.h"
#include "univ/str.h"
#include "univ/system.h"

static ByteVec names;
static HashMap map;

void InitSymbols(void)
{
  InitVec(&names, sizeof(u8), 0);
  InitHashMap(&map);
}

u32 AddSymbol(char *name)
{
  u32 len = StrLen(name);
  return AddSymbolLen(name, len);
}

u32 AddSymbolLen(char *name, u32 len)
{
  u32 hash = Hash(name, len);

  if (!HashMapContains(&map, hash)) {
    u32 old_count = GrowVec(&names, sizeof(u8), len);
    HashMapSet(&map, hash, old_count);
    Copy(name, &VecRef(&names, old_count), len);
    ByteVecPush(&names, 0);
  }

  return hash;
}

char *SymbolName(u32 sym)
{
  if (!HashMapContains(&map, sym)) return 0;
  return (char*)names.items + HashMapGet(&map, sym);
}
