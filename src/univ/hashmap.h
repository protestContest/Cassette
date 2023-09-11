#pragma once

typedef struct MapBucket MapBucket;

typedef struct HashMap {
  u32 capacity;
  u32 count;
  MapBucket *buckets;
} HashMap;

#define EmptyHashMap  ((HashMap){0, 0, NULL})

void DestroyHashMap(HashMap *map);
void HashMapSet(HashMap *map, u32 key, u32 value);
bool HashMapContains(HashMap *map, u32 key);
u32 HashMapGet(HashMap *map, u32 key);
void HashMapDelete(HashMap *map, u32 key);
u32 HashMapKey(HashMap *map, u32 key_num);

