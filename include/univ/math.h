#pragma once

#define MinInt            ((i32)0x80000000)
#define MaxInt            ((i32)0x7FFFFFFF)
#define MaxUInt           ((u32)0xFFFFFFFF)
#define Abs(n)            (((n) ^ ((n) >> 31)) - ((n) >> 31))
#define Align(n, m)       ((n) == 0 ? 0 : ((((n) - 1) / (m) + 1) * (m)))
#define RotL(n, b)        ((((u32)(n)) << (b)) | (((u32)(n)) >> (32 - (b))))
#define RotR(n, b)        ((((u32)(n)) >> (b)) | (((u32)(n)) << (32 - (b))))
#define RightZeroBit(x)   (~(x) & ((x) + 1))
#define SwapBytes(n)      ((((n)<<24)&0xFF000000)|\
                           (((n)<< 8)&0x00FF0000)|\
                           (((n)>> 8)&0x0000FF00)|\
                           (((n)>>24)&0x000000FF))

bool IsBigEndian(void);

u32 NumDigits(i32 num, u32 base);
void SeedRandom(u32 seed);
u32 Random(void);
u32 PopCount(u32 num);

/* Returns the number of bytes num needs to be encoded as a LEB */
i32 LEBSize(i32 num);

/* Writes a number into a buffer as a LEB */
void WriteLEB(i32 num, u32 pos, u8 *buf);

/* Reads a LEB number from a buffer */
i32 ReadLEB(u32 index, u8 *buf);

/* DJB2 hash */
#define EmptyHash 5381
u32 Hash(void *data, u32 size);
#define HashStr(str) Hash(str, StrLen(str))
u32 AppendHash(u32 hash, void *data, u32 size);
u32 FoldHash(u32 hash, i32 size);

u32 ByteSwap(u32 n);
u16 ByteSwapShort(u16 n);
u32 CRC32(u8 *data, u32 size);

/* Crockford's version. Resulting string is zero-padded and null-terminated. */
char *Base32Encode(void *data, u32 len);

/* Crockford's version. Invalid input will return null. */
char *Base32Decode(void *data, u32 len);
