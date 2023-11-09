#include "math.h"

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
  u64 xorshifted, rot;
  rand_state = oldstate * 6364136223846793005 + 1;
  xorshifted = ((oldstate >> 18) ^ oldstate) >> 27;
  rot = oldstate >> 59;
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
