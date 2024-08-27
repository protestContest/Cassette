#pragma once

/* A basic hashmap with robin hood hashing */

typedef struct {
  u32 capacity;
  u32 count;
  struct MapBucket *buckets;
} HashMap;

#define EmptyHashMap {0, 0, 0}

void InitHashMap(HashMap *map);
void DestroyHashMap(HashMap *map);
void HashMapSet(HashMap *map, u32 key, u32 value);
bool HashMapContains(HashMap *map, u32 key);
u32 HashMapGet(HashMap *map, u32 key);
void HashMapDelete(HashMap *map, u32 key);
u32 HashMapKey(HashMap *map, u32 key_num);
