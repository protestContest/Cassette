#include "hashmap.h"
#include "hash.h"
#include "math.h"
#include "memory.h"

typedef struct MapBucket {
  u32 key;
  u32 value;
  i32 probe_count;
} MapBucket;

#define IndexFor(hash, cap)   ((hash) & ((cap) - 1))
#define BucketIsEmpty(bucket) ((bucket).probe_count < 0)
#define MapTooSmall(map)      ((map)->capacity == 0 || (map)->count > (((map)->capacity >> 2) + ((map)->capacity >> 1)))
#define MapTooBig(map)        ((map)->capacity > 32 && (map)->count < ((map)->capacity >> 2))

static void ResizeMap(HashMap *map, u32 capacity)
{
  capacity = Max(capacity, 32);
  HashMap map2 = (HashMap){capacity, map->count, Allocate(sizeof(MapBucket)*capacity)};
  for (u32 i = 0; i < capacity; i++) map2.buckets[i].probe_count = -1;
  for (u32 i = 0; i < map->capacity; i++) {
    if (!BucketIsEmpty(map->buckets[i])) {
      HashMapSet(&map2, map->buckets[i].key, map->buckets[i].value);
      map2.count = map->count;
    }
  }
  DestroyHashMap(map);
  *map = map2;
}

void DestroyHashMap(HashMap *map)
{
  if (map->buckets) Free(map->buckets);
}

void HashMapSet(HashMap *map, u32 key, u32 value)
{
  if (MapTooSmall(map)) ResizeMap(map, 2*map->capacity);

  u32 hash = Hash(&key, sizeof(key));
  u32 index = IndexFor(hash, map->capacity);
  MapBucket bucket = (MapBucket){key, value, 0};

  while (true) {
    if (BucketIsEmpty(map->buckets[index])) {
      map->buckets[index] = bucket;
      map->count++;
      return;
    }
    if (map->buckets[index].key == key) {
      map->buckets[index].value = value;
      return;
    }
    if (map->buckets[index].probe_count < bucket.probe_count) {
      MapBucket tmp = map->buckets[index];
      map->buckets[index] = bucket;
      bucket = tmp;
    }

    index = IndexFor(index + 1, map->capacity);
    bucket.probe_count++;
  }
}

bool HashMapContains(HashMap *map, u32 key)
{
  if (map->capacity == 0) return false;
  u32 hash = Hash(&key, sizeof(key));
  u32 index = hash % map->capacity;

  while (true) {
    if (BucketIsEmpty(map->buckets[index])) return false;
    if (map->buckets[index].key == key) return true;
    index = IndexFor(index + 1, map->capacity);
  }
}

u32 HashMapGet(HashMap *map, u32 key)
{
  if (map->capacity == 0) return -1;

  u32 hash = Hash(&key, sizeof(key));
  u32 index = hash % map->capacity;

  while (true) {
    if (map->buckets[index].key == key) return map->buckets[index].value;
    if (BucketIsEmpty(map->buckets[index])) return -1;
    index = IndexFor(index + 1, map->capacity);
  }
}

void HashMapDelete(HashMap *map, u32 key)
{
  if (map->capacity == 0) return;
  u32 hash = Hash(&key, sizeof(key));
  u32 index = hash % map->capacity;

  while (true) {
    if (BucketIsEmpty(map->buckets[index])) return;
    if (map->buckets[index].key == key) {
      map->buckets[index].probe_count = -1;
      u32 next_index = IndexFor(index + 1, map->capacity);
      while (map->buckets[next_index].probe_count > 0) {
        map->buckets[index] = map->buckets[next_index];
        index = next_index;
        next_index = IndexFor(next_index + 1, map->capacity);
      }
      map->count--;
      if (MapTooBig(map)) ResizeMap(map, map->capacity/2);
      return;
    }
    index = IndexFor(index + 1, map->capacity);
  }
}

u32 HashMapKey(HashMap *map, u32 key_num)
{
  u32 count = 0;
  for (u32 i = 0; i < map->capacity; i++) {
    if (!BucketIsEmpty(map->buckets[i])) {
      if (count == key_num) return map->buckets[i].key;
      count++;
    }
  }
  return EmptyHash;
}
