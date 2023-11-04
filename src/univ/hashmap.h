#pragma once

typedef struct MapBucket {
  u32 key;
  u32 value;
  i32 probe;
} MapBucket;

typedef struct {
  u32 capacity;
  u32 count;
  struct MapBucket *buckets;
} HashMap;

void InitHashMap(HashMap *map);
void DestroyHashMap(HashMap *map);
void HashMapSet(HashMap *map, u32 key, u32 value);
bool HashMapContains(HashMap *map, u32 key);
u32 HashMapGet(HashMap *map, u32 key);
void HashMapDelete(HashMap *map, u32 key);
u32 HashMapKey(HashMap *map, u32 key_num);
