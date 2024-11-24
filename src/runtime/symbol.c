#include "runtime/symbol.h"
#include "univ/hashmap.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/vec.h"

static i32 symSize = 32;
static char *names = 0; /* vec */
static HashMap map = EmptyHashMap;

u32 Symbol(char *name)
{
  u32 len = StrLen(name);
  return SymbolFrom(name, len);
}

u32 SymbolFrom(char *name, u32 len)
{
  u32 sym = FoldHash(Hash(name, len), symSize);
  if (!HashMapContains(&map, sym)) {
    u32 index = VecCount(names);
    HashMapSet(&map, sym, index);
    GrowVec(names, len);
    Copy(name, names + index, len);
    VecPush(names, 0);
  }
  return sym;
}

char *SymbolName(u32 sym)
{
  if (!HashMapContains(&map, sym)) return 0;
  return names + HashMapGet(&map, sym);
}

void SetSymbolSize(i32 size)
{
  symSize = size;
}
