#pragma once

#define Abs(n)            (((n) ^ ((n) >> 31)) - ((n) >> 31))
#define Align(n, m)       ((n) == 0 ? 0 : ((((n) - 1) / (m) + 1) * (m)))
#define RightZeroBit(x)   (~(x) & ((x) + 1))

u32 NumDigits(i32 num, u32 base);
void SeedRandom(u32 seed);
u32 Random(void);
u32 PopCount(u32 num);
