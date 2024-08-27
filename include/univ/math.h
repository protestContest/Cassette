#pragma once

#define MinInt            ((i32)0x80000000)
#define MaxInt            ((i32)0x7FFFFFFF)
#define MaxUInt           ((u32)0xFFFFFFFF)
#define Abs(n)            (((n) ^ ((n) >> 31)) - ((n) >> 31))
#define Min(a, b)         ((a) ^ (((b) ^ (a)) & -((b) < (a))))
#define Max(a, b)         ((b) ^ (((b) ^ (a)) & -((b) < (a))))
#define Align(n, m)       ((((n) - 1) / (m) + 1) * (m))
#define Bit(n)            ((i64)1 << (n))
#define RightZeroBit(x)   (~(x) & ((x) + 1))

u32 NumDigits(i32 num, u32 base);
void SeedRandom(u32 seed);
u32 Random(void);
i32 RandomBetween(i32 min, i32 max);
