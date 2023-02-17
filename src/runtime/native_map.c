#include "native_map.h"
#include "../platform/allocate.h"

void InitNativeMap(NativeMap *map)
{
  map->count = 0;
  map->capacity = 64;
  map->items = Allocate(sizeof(NativeMapItem)*map->capacity);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].name = nil;
  }
}

static void ResizeNativeMap(NativeMap *map, u32 capacity)
{
  map->capacity = capacity;
  map->items = Reallocate(map->items, capacity*sizeof(NativeMapItem));
}

void NativeMapPut(NativeMap *map, Val key, NativeFn impl)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      map->items[index].impl = impl;
      return;
    }
    index = (index + 1) % map->capacity;
  }

  map->items[index].name = key;
  map->items[index].impl = impl;
  map->count++;

  if (map->count > 0.8*map->capacity) {
    ResizeNativeMap(map, 2*map->capacity);
  }
}

NativeFn NativeMapGet(NativeMap *map, Val key)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      return map->items[index].impl;
    }
  }

  return NULL;
}
