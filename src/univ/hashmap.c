#include "hashmap.h"
#include "hash.h"
#include "math.h"
#include "system.h"

#define IndexFor(hash, cap)   ((hash) & ((cap) - 1))
#define IsEmpty(bucket)       ((bucket).probe < 0)
#define MaxBuckets(cap)       (((map)->capacity >> 2) + ((map)->capacity >> 1))
#define MinBuckets(cap)       ((cap) >> 2)
#define TooSmall(map)         ((map)->capacity == 0 || (map)->count > MaxBuckets((map)->capacity))
#define TooBig(map)           ((map)->capacity > 32 && (map)->count < MinBuckets((map)->capacity))

static bool Put(HashMap *map, u32 key, u32 value);

static void ResizeMap(HashMap *map, u32 capacity)
{
  HashMap map2;
  u32 i;

  map2.capacity = Max(capacity, 32);
  map2.count = map->count;
  map2.buckets = Alloc(sizeof(MapBucket)*map2.capacity);

  for (i = 0; i < map2.capacity; i++) map2.buckets[i].probe = -1;
  for (i = 0; i < map->capacity; i++) {
    if (!IsEmpty(map->buckets[i])) {
      u32 key = map->buckets[i].key;
      u32 value = map->buckets[i].value;
      Put(&map2, key, value);
    }
  }

  if (map->buckets) Free(map->buckets);
  map->capacity = map2.capacity;
  map->buckets = map2.buckets;
}

void InitHashMap(HashMap *map)
{
  map->capacity = 0;
  map->count = 0;
  map->buckets = 0;
}

void DestroyHashMap(HashMap *map)
{
  if (map->buckets) Free(map->buckets);
  map->buckets = 0;
  map->count = 0;
  map->capacity = 0;
}

static bool Put(HashMap *map, u32 key, u32 value)
{
  u32 hash = Hash(&key, sizeof(key));
  u32 index = IndexFor(hash, map->capacity);
  MapBucket bucket;
  bucket.key = key;
  bucket.value = value;
  bucket.probe = 0;

  while (true) {
    if (IsEmpty(map->buckets[index])) {
      map->buckets[index] = bucket;
      return true;
    }
    if (map->buckets[index].key == key) {
      map->buckets[index].value = value;
      return false;
    }
    if (map->buckets[index].probe < bucket.probe) {
      MapBucket tmp = map->buckets[index];
      map->buckets[index] = bucket;
      bucket = tmp;
    }

    index = IndexFor(index + 1, map->capacity);
    bucket.probe++;
  }
}

void HashMapSet(HashMap *map, u32 key, u32 value)
{
  if (TooSmall(map)) ResizeMap(map, 2*map->capacity);
  if (Put(map, key, value)) map->count++;
}

bool HashMapContains(HashMap *map, u32 key)
{
  u32 hash, index;
  if (map->capacity == 0) return false;

  hash = Hash(&key, sizeof(key));
  index = IndexFor(hash, map->capacity);

  while (true) {
    if (IsEmpty(map->buckets[index])) return false;
    if (map->buckets[index].key == key) return true;
    index = IndexFor(index + 1, map->capacity);
  }
}

u32 HashMapGet(HashMap *map, u32 key)
{
  u32 hash, index;
  if (map->capacity == 0) return -1;

  hash = Hash(&key, sizeof(key));
  index = IndexFor(hash, map->capacity);

  while (true) {
    if (IsEmpty(map->buckets[index])) return -1;
    if (map->buckets[index].key == key) return map->buckets[index].value;
    index = IndexFor(index + 1, map->capacity);
  }
}

void HashMapDelete(HashMap *map, u32 key)
{
  u32 hash, index;
  if (map->capacity == 0) return;

  hash = Hash(&key, sizeof(key));
  index = IndexFor(hash, map->capacity);

  while (true) {
    if (IsEmpty(map->buckets[index])) return;
    if (map->buckets[index].key == key) {
      u32 next_index = IndexFor(index + 1, map->capacity);
      map->buckets[index].probe = -1;
      while (map->buckets[next_index].probe > 0) {
        map->buckets[index] = map->buckets[next_index];
        map->buckets[index].probe--;
        index = next_index;
        next_index = IndexFor(next_index + 1, map->capacity);
      }
      map->count--;
      /* if (TooBig(map)) ResizeMap(map, map->capacity/2);*/
      return;
    }
    index = IndexFor(index + 1, map->capacity);
  }
}

u32 HashMapKey(HashMap *map, u32 key_num)
{
  u32 i, count = 0;
  for (i = 0; i < map->capacity; i++) {
    if (!IsEmpty(map->buckets[i])) {
      if (count == key_num) return map->buckets[i].key;
      count++;
    }
  }
  return 0;
}
