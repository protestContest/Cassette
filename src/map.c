#include "map.h"
#include <univ/allocate.h>

void ResizeMap(void *map, u32 capacity, u32 item_size)
{
  ((Map*)map)->capacity = capacity;
  ((Map*)map)->items = Reallocate(((Map*)map)->items, capacity*item_size);
}

void InitMap(void *map, u32 item_size)
{
  ((Map*)map)->count = 0;
  ((Map*)map)->capacity = 0;
  ((Map*)map)->items = NULL;
  ResizeMap(map, 32, item_size);
}

void FreeMap(void *map)
{
  ((Map*)map)->count = 0;
  ((Map*)map)->capacity = 0;
  Free(((Map*)map)->items);
}
