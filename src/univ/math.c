#include "math.h"
#include <math.h>

/* Based on this PCG algorithm: https://github.com/imneme/pcg-c-basic */

static u64 rand_state = 0;

void Seed(u32 seed)
{
  rand_state = 0;
  Random();
  rand_state += seed;
  Random();
}

u32 Random(void)
{
  u64 oldstate = rand_state;
  rand_state = oldstate * 6364136223846793005 + 1;
  u32 xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
  u32 rot = oldstate >> 59;
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

u32 PopCount(u32 n)
{
  n = n - ((n >> 1) & 0x55555555);
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
  n = (n + (n >> 4)) & 0x0F0F0F0F;
  n = n + (n >> 8);
  n = n + (n >> 16);
  return n & 0x0000003F;
}

f32 Sin(f32 r)
{
  return sinf(r);
}

f32 Cos(f32 r)
{
  return cosf(r);
}

f32 Tan(f32 r)
{
  return tanf(r);
}

f32 Log(f32 x)
{
  return logf(x);
}

f32 Exp(f32 x)
{
  return expf(x);
}

f32 Sqrt(f32 x)
{
  return sqrt(x);
}

f32 Pow(f32 b, f32 e)
{
  return powf(b, e);
}
