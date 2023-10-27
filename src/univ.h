#pragma once

/****************************\
*    __  __ _   __ __ _   __ *
*   / / / // | / // /| | / / *
*  / / / //  |/ // / | |/ /  *
* / /_/ // /|  // /  |   /   *
* \____//_/ |_//_/   |__/    *
*                            *
\****************************/

#define EmptyHash           ((u32)0x97058A1C)

u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, void *data, u32 size);
u32 HashBits(void *data, u32 size, u32 num_bits);


typedef struct MapBucket {
  u32 key;
  u32 value;
  i32 probe;
} MapBucket;

typedef struct {
  u32 capacity;
  u32 count;
  struct MapBucket **buckets;
} HashMap;

void InitHashMap(HashMap *map);
void DestroyHashMap(HashMap *map);
void HashMapSet(HashMap *map, u32 key, u32 value);
bool HashMapContains(HashMap *map, u32 key);
u32 HashMapGet(HashMap *map, u32 key);
void HashMapDelete(HashMap *map, u32 key);
u32 HashMapKey(HashMap *map, u32 key_num);

#define Pi              3.141593
#define E               2.718282
#define Epsilon         1e-6
#define Sqrt_Two        1.414213
#define Infinity        ((float)0x7F800000)
#define MinInt          ((i32)0x80000000)
#define MaxInt          ((i32)0x7FFFFFFF)
#define MaxUInt         ((u32)0xFFFFFFFF)

#define Bit(n)          (1 << (n))
#define Abs(x)          ((x)*(((x) > 0) - ((x) < 0)))
#define Min(a, b)       ((a) < (b) ? (a) : (b))
#define Max(a, b)       ((a) > (b) ? (a) : (b))
#define Ceil(x)         ((i32)(x) + ((x) > (i32)(x)))
#define Floor(x)        ((i32)(x) - ((x) < (i32)(x)))
#define Round(x)        Floor((x) + 0.5)
#define Clamp(x, a, b)  Min(Max((x), a), b)
#define Lerp(a, b, w)   (((b) - (a))*(w) + (a))
#define Norm(a, b, w)   (((w) - (a)) / ((b) - (a)))

void Seed(u32 seed);
u32 Random(void);
u32 PopCount(u32 n);

u32 StrLen(char *str);
bool StrEq(char *str1, char *str2);

#define Assert(expr) ((expr) || (Alert("Assert failed in " __FILE__ ": " #expr), Exit(), 0))
typedef void **Handle;

Handle NewHandle(u32 size);
void SetHandleSize(Handle handle, u32 size);
void DisposeHandle(Handle handle);
void Copy(void *src, void *dst, u32 size);
void Exit(void);
void Alert(char *message);
char *ReadFile(char *path);
