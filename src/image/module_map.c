#include "module_map.h"
#include "../map.h"

#define ItemSize  sizeof(ModuleMapItem)

void InitModuleMap(ModuleMap *map)
{
  InitMap(map, ItemSize);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].name = nil;
  }
}

void PutModule(ModuleMap *map, Val key, Module *module)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      map->items[index].module = module;
      return;
    }
    index = (index + 1) % map->capacity;
  }

  map->items[index].name = key;
  map->items[index].module = module;
  map->count++;

  if (map->count > 0.8*map->capacity) {
    ResizeMap(map, 2*map->capacity, ItemSize);
  }
}

Module *GetModule(ModuleMap *map, Val key)
{
  u32 index = RawSym(key) % map->capacity;
  while (!IsNil(map->items[index].name)) {
    if (Eq(map->items[index].name, key)) {
      return map->items[index].module;
    }
  }

  return NULL;
}

#undef ItemSize
