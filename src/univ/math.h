#pragma once

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
