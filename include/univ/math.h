#pragma once

#define MinInt            ((i32)0x80000000)
#define MaxInt            ((i32)0x7FFFFFFF)
#define MaxUInt           ((u32)0xFFFFFFFF)
#define Abs(n)            (((n) ^ ((n) >> 31)) - ((n) >> 31))
#define Align(n, m)       ((n) == 0 ? 0 : ((((n) - 1) / (m) + 1) * (m)))
#define RightZeroBit(x)   (~(x) & ((x) + 1))

u32 NumDigits(i32 num, u32 base);
void SeedRandom(u32 seed);
u32 Random(void);
u32 PopCount(u32 num);

i32 LEBSize(i32 num);
void WriteLEB(i32 num, u32 pos, u8 *buf);
i32 ReadLEB(u32 index, u8 *buf);

#define EmptyHash 5381
u32 Hash(void *data, u32 size);
u32 AppendHash(u32 hash, void *data, u32 size);
u32 FoldHash(u32 hash, i32 size);

u32 ByteSwap(u32 n);
