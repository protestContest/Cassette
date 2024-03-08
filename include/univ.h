#pragma once

u32 NumDigits(u32 num, u32 base);
u32 PopCount(u32 n);
#pragma once

#define ANSIRed         "\033[31m"
#define ANSIUnderline   "\033[4m"
#define ANSINormal      "\033[0m"

char *SkipSpaces(char *str);
char *LineEnd(char *str);
u32 LineNum(char *str, u32 index);
u32 ColNum(char *str, u32 index);
i32 FindString(char *needle, u32 nlen, char *haystack, u32 hlen);
char *JoinStr(char *str1, char *str2, char joiner);
u32 ParseInt(char *str);
#pragma once

u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, u8 byte);
u32 SkipHash(u32 hash, u8 byte, u32 size);
u32 FoldHash(u32 hash, u32 bits);
#pragma once

void HexDump(void *data, u32 size);
#pragma once

#define NewVec(type, max)     ResizeVec(0, max, sizeof(type))
#define FreeVec(vec)          ((vec) ? free(RawVec(vec)),0 : 0)
#define VecCapacity(vec)      ((vec) ? RawVecCap(vec) : 0)
#define VecCount(vec)         ((vec) ? RawVecCount(vec) : 0)
#define VecPush(vec, val)     (VecMakeRoom(vec, 1), (vec)[RawVecCount(vec)++] = val)
#define VecPop(vec)           (VecCount(vec) > 0 ? RawVecCount(vec)-- : 0)
#define GrowVec(vec, num)     (VecMakeRoom(vec, num), RawVecCount(vec) += num)
#define VecEnd(vec)           &(vec[RawVecCount(vec)])

#define RawVec(vec)           (((u32 *)vec) - 2)
#define RawVecCap(vec)        RawVec(vec)[0]
#define RawVecCount(vec)      RawVec(vec)[1]
#define VecHasRoom(vec, n)    (VecCount(vec) + (n) < VecCapacity(vec))
#define VecMakeRoom(vec, n)   (VecHasRoom(vec, n) ? 0 : DoVecGrow(vec, n))
#define DoVecGrow(vec, n)     (*((void **)&(vec)) = ResizeVec((vec), (n), sizeof(*(vec))/* NOLINT */))

void *ResizeVec(void *vec, u32 num_items, u32 item_size);
#pragma once

u32 AddSymbol(char *name);
u32 AddSymbolLen(char *name, u32 len);
char *SymbolName(u32 sym);
#pragma once

char *ReadFile(char *path);
#pragma once

/* A basic hashmap with robin hood hashing */

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

#define EmptyHashMap {0, 0, 0}

void DestroyHashMap(HashMap *map);
void HashMapSet(HashMap *map, u32 key, u32 value);
bool HashMapContains(HashMap *map, u32 key);
u32 HashMapGet(HashMap *map, u32 key);
void HashMapDelete(HashMap *map, u32 key);
u32 HashMapKey(HashMap *map, u32 key_num);
