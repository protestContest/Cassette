#pragma once

/* A basic hash map with robin hood hashing. Note that keys are not hashed. */

typedef struct {
  u32 capacity;
  u32 count;
  struct MapBucket *buckets;
} HashMap;

#define EmptyHashMap {0, 0, 0}

/* Sets a hash map to the empty hash map */
void InitHashMap(HashMap *map);

/* Destroys a hash map */
void DestroyHashMap(HashMap *map);

/* Sets a value in a hash map for a key */
void HashMapSet(HashMap *map, u32 key, u32 value);

/* Returns whether a hash map contains a key */
bool HashMapContains(HashMap *map, u32 key);

/* Returns the value of a key in a hash map, or -1 if not present */
u32 HashMapGet(HashMap *map, u32 key);

/* Returns whether a hashmap contains a key, and the value if true. */
bool HashMapFetch(HashMap *map, u32 key, u32 *value);

/* Deletes an entry from a hash map */
void HashMapDelete(HashMap *map, u32 key);

/* Returns the nth key in a hash map. Useful for iterating over all entries. */
u32 HashMapKey(HashMap *map, u32 n);
