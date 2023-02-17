#include "module_map.h"

void InitModuleMap(ModuleMap *map)
{
  map->count = 0;
  map->capacity = 64;
  map->items = malloc(sizeof(ModuleMapItem)*map->capacity);
  for (u32 i = 0; i < map->capacity; i++) {
    map->items[i].name = nil;
  }
}

static void ResizeModuleMap(ModuleMap *map, u32 capacity)
{
  map->capacity = capacity;
  map->items = realloc(map->items, capacity*sizeof(ModuleMapItem));
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
    ResizeModuleMap(map, 2*map->capacity);
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
