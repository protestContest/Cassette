/* base.h */
#pragma once

#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef unsigned char byte;

#define NULL 0
#define bool _Bool
#define true 1
#define false 0

#define Bit(n)    (1 << (n))
#define ArrayCount(a)   (sizeof (a) / sizeof (a)[0])

#define Assert(c) if (!(c)) __builtin_trap()

#define unused __attribute__((unused))
/* buf.h */
#pragma once

#define MemBuf(data, cap)       { 0, cap, (u8*)data, -1, false }
#define IOBuf(file, data, cap)  { 0, cap, data, file, false }

typedef struct Buf {
  u32 count;
  u32 capacity;
  u8 *data;
  int file;
  bool error;
} Buf;

u32 Append(Buf *buf, u8 *src, u32 len);
#define AppendStr(buf, str) Append(buf, (u8 *)str, sizeof(str) - 1)
u32 AppendByte(Buf *buf, u8 byte);
u32 AppendInt(Buf *buf, i32 num);
u32 AppendFloat(Buf *buf, float num);
void Flush(Buf *buf);

u32 IntToString(i32 num, char *str, u32 max);
u32 FloatToString(float num, char *str, u32 max);
/* hash.h */
#pragma once

u32 EmptyHash(void);
u32 Hash(void *data, u32 size);
u32 AddHash(u32 hash, void *data, u32 size);
u32 HashBits(void *data, u32 size, u32 num_bits);
/* io.h */
#pragma once

struct Buf;
extern struct Buf *output;

#define IOReset         "\33[0m"
#define IOBold          "\33[1m"
#define IONoBold        "\33[22m"
#define IOUnderline     "\33[4m"
#define IONoUnderline   "\33[24m"
#define IOFGRed         "\33[31m"
#define IOFGReset       "\33[39m"

void Print(char *str);
void PrintLn(char *str);
void PrintN(char *str, u32 size);
void PrintInt(i32 num);
void PrintIntN(i32 num, u32 size);
void PrintFloat(float num);
void FlushIO(void);

char *ReadLine(char *buf, u32 size);
char *ReadFile(char *path);
/* map.h */
#pragma once

typedef struct MapBucket MapBucket;

typedef struct Map {
  u32 capacity;
  u32 count;
  MapBucket *buckets;
} Map;

void InitMap(Map *map);
void FreeMap(Map *map);
void MapSet(Map *map, u32 key, void *value);
bool MapContains(Map *map, u32 key);
void *MapGet(Map *map, u32 key);
void MapDelete(Map *map, u32 key);
/* math.h */
#pragma once

#define Pi              3.14159265359
#define Epsilon         1e-5
#define Sqrt_Two        1.41421356237
#define Infinity        ((float)0x7F800000)
#define MinInt          ((i32)0x80000000)
#define MaxInt          ((i32)0x7FFFFFFF)

#define Abs(x)          ((x)*(((x) > 0) - ((x) < 0)))
#define Min(a, b)       ((a) < (b) ? (a) : (b))
#define Max(a, b)       ((a) > (b) ? (a) : (b))
#define Ceil(x)         ((i32)(x) + ((x) > 0))
#define Floor(x)        ((i32)(x) - ((x) < 0))
#define Clamp(x, a, b)  min(max((x), a), b)
#define Lerp(a, b, w)   (((b) - (a))*(w) + (a))
#define Norm(a, b, w)   (((w) - (a)) / ((b) - (a)))
/* str.h */
#pragma once

u32 StrLen(char *str);
u32 NumDigits(i32 num);
/* sys.h */
#pragma once

void *Allocate(u32 size);
void *Reallocate(void *ptr, u32 size);
void Free(void *ptr);
bool Read(int file, void *data, u32 length);
bool Write(int file, void *data, u32 length);
void Error(char *message);
void Exit(void);
/* vec.h */
#pragma once

// a vec is an array prepended with two ints in memory for capacity and count

#define NewVec(type, max)     ResizeVec(0, max, sizeof(type))
#define FreeVec(vec)          ((vec) ? Free(RawVec(vec)),0 : 0)
#define VecCapacity(vec)      ((vec) ? RawVecCapacity(vec) : 0)
#define VecCount(vec)         ((vec) ? RawVecCount(vec) : 0)
#define VecPush(vec, value)   (VecMaybeGrow(vec, 1), (vec)[RawVecCount(vec)++] = value)
#define VecPop(vec, default)  ((vec) && VecCount(vec) > 0 ? (vec)[--RawVecCount(vec)] : default)
#define VecEnd(vec)           &((vec)[RawVecCount(vec)])
#define VecNext(vec)          (VecMaybeGrow(vec, 1), &(vec[RawVecCount(vec)++]))
#define VecPeek(vec, n)       ((vec)[VecCount(vec) - 1 - (n)])
#define VecLast(vec)          VecPeek(vec, 0)
#define EmptyVec(vec)         ((vec) ? RawVecCount(vec) = 0 : 0)
#define RewindVec(vec, num)   RawVecCount(vec) -= (VecCount(vec) < (num) ? VecCount(vec) : (num))
#define GrowVec(vec, num)     (VecMaybeGrow(vec, num), RawVecCount(vec) += num)

#define RawVec(vec)           (((u32 *)vec) - 2)
#define UnrawVec(vec, type)   (type *)(vec + 2)
#define RawVecCapacity(vec)   RawVec(vec)[0]
#define RawVecCount(vec)      RawVec(vec)[1]
#define VecHasRoom(vec, n)    (VecCount(vec) + (n) <= VecCapacity(vec))
#define VecMaybeGrow(vec, n)  (VecHasRoom(vec, n) ? 0 : VecGrow(vec, n))
#define VecGrow(vec, n)       (*((void **)&(vec)) = ResizeVec((vec), (n), sizeof(*(vec))/* NOLINT */))

u32 *NewRawVec(u32 item_size, u32 max_items);
void *ResizeVec(void *vec, u32 num_items, u32 item_size);
void *AppendVec(void *dst, void *src, u32 item_size);
