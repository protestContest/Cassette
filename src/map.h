#pragma once

typedef struct {
  u32 capacity;
  u32 count;
  void *items;
} Map;

void ResizeMap(void *map, u32 capacity, u32 item_size);
void InitMap(void *map, u32 item_size);
void FreeMap(void *map);
