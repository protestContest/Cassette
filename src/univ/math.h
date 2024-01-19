#pragma once

#include <stdlib.h>
#include <math.h>

#define Pi              3.141593
#define E               2.718282
#define Epsilon         1e-6
#define Sqrt_Two        1.414213
#define Infinity        ((float)0x7F800000)
#define MinInt          ((i32)0x80000000)
#define MaxInt          ((i32)0x7FFFFFFF)
#define MaxUInt         ((u32)0xFFFFFFFF)

#define Bit(n)          (1 << (n))
#define Abs(n)          fabsf((float)n)
#define Min(a, b)       ((a) < (b) ? (a) : (b))
#define Max(a, b)       ((a) > (b) ? (a) : (b))
#define Ceil(n)         ceilf(n)
#define Floor(n)        floorf(n)
#define Round(n)        lroundf(n)
#define Log10(n)        log10(n)
#define NumDigits(n)    Floor(Log10(Abs(n))) + 1
#define Align(n, m)     ((((n) - 1) / (m) + 1) * (m))

#define Seed(seed)      srandom(seed)
#define Random()        random()
#define RightZeroBit(x) (~(x) & ((x) + 1))

u32 PopCount(u32 n);
