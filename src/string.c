#include "string.h"
#include "hash.h"

static void ResizeStringMap(StringMap *map, u32 capacity)
{
  map->capacity = capacity;
  map->items = realloc(map->items, capacity*sizeof(Val));
}

void InitStringMap(StringMap *map)
{
  map->count = 0;
  map->capacity = 0;
  map->items = NULL;
  ResizeStringMap(map, 32);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].key = nil;
  }
}

void FreeStringMap(StringMap *map)
{
  map->count = 0;
  map->capacity = 0;
  free(map->items);
}

Val PutString(StringMap *map, char *str, u32 length)
{
  u32 hash = HashSize(str, length, objBits);
  u32 index = hash % map->capacity;
  Val val = BinVal(hash);

  while (!IsNil(map->items[index].key)) {
    if (Eq(map->items[index].key, val)) return val;

    index = (index + 1) % map->capacity;
  }

  map->items[index].key = val;
  map->items[index].length = length;
  map->items[index].data = malloc(length);
  for (u32 i = 0; i < length; i++) {
    map->items[index].data[i] = str[i];
  }

  map->count++;
  if (map->count > 0.8*map->capacity) {
    ResizeStringMap(map, 2*map->capacity);
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
  return MakeSymbolFromSlice(map, src, strlen(src));
}

Val MakeSymbolFromSlice(StringMap *map, char *src, u32 len)
{
  Val str = PutString(map, src, len);
  return SymVal(RawObj(str));
}

Val SymbolFor(char *src)
{
  u32 hash = HashSize(src, strlen(src), objBits);
  return SymVal(hash);
}

char *SymbolName(StringMap *map, Val sym)
{
  Val str = BinVal(RawObj(sym));
  return StringData(map, str);
}

