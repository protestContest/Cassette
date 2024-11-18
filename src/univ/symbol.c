#include "univ/symbol.h"
#include "univ/hash.h"
#include "univ/hashmap.h"
#include "univ/vec.h"
#include "univ/str.h"

static i32 symSize = 32;
static VecOf(char) **names = 0;
static HashMap map = EmptyHashMap;

u32 Symbol(char *name)
{
  u32 len = strlen(name);
  return SymbolFrom(name, len);
}

u32 SymbolFrom(char *name, u32 len)
{
  u32 sym = FoldHash(Hash(name, len), symSize);
  if (!names) InitVec(names);
  if (!HashMapContains(&map, sym)) {
    u32 index = VecCount(names);
    HashMapSet(&map, sym, index);
    GrowVec(names, len);
    Copy(name, VecData(names) + index, len);
    VecPush(names, 0);
  }
  return sym;
}

char **SymbolName(u32 sym)
{
  if (!HashMapContains(&map, sym)) return 0;
  return NewString(VecData(names) + HashMapGet(&map, sym));
}

void SetSymbolSize(i32 size)
{
  symSize = size;
}

bool SymbolExists(u32 sym)
{
  return HashMapContains(&map, sym);
}
