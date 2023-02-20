#include "string_map.h"
#include <univ/hash.h>
#include "../map.h"
#include <univ/allocate.h>
#include <univ/str.h>

#define ItemSize  sizeof(Val)

void InitStringMap(StringMap *map)
{
  InitMap(map, ItemSize);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].key = nil;
  }
}

Val PutString(StringMap *map, char *str, u32 length)
{
  u32 hash = Hash(str, length);
  u32 index = hash % map->capacity;
  Val val = BinVal(hash);

  while (!IsNil(map->items[index].key)) {
    if (Eq(map->items[index].key, val)) return val;

    index = (index + 1) % map->capacity;
  }

  map->items[index].key = val;
  map->items[index].length = length;
  map->items[index].data = Allocate(length);
  for (u32 i = 0; i < length; i++) {
    map->items[index].data[i] = str[i];
  }

  map->count++;
  if (map->count > 0.8*map->capacity) {
    ResizeMap(map, 2*map->capacity, ItemSize);
  }

  return val;
}

u32 StringLength(StringMap *map, Val str)
{
  u32 index = RawObj(str) % map->capacity;

  while (!IsNil(map->items[index].key)) {
    if (Eq(map->items[index].key, str)) return map->items[index].length;
    index = (index + 1) % map->capacity;
  }

  return 0;
}

char *StringData(StringMap *map, Val str)
{
  u32 index = RawObj(str) % map->capacity;

  while (!IsNil(map->items[index].key)) {
    if (Eq(map->items[index].key, str)) return map->items[index].data;
    index = (index + 1) % map->capacity;
  }

  return NULL;
}

Val MakeSymbol(StringMap *map, char *src)
{
  return MakeSymbolFromSlice(map, src, StrLen(src));
}

Val MakeSymbolFromSlice(StringMap *map, char *src, u32 len)
{
  Val str = PutString(map, src, len);
  return SymVal(RawObj(str));
}

Val SymbolFor(char *src)
{
  u32 hash = Hash(src, StrLen(src));
  return SymVal(hash);
}

char *SymbolName(StringMap *map, Val sym)
{
  Val str = BinVal(RawObj(sym));
  return StringData(map, str);
}

#undef ItemSize
