#include "univ/math.h"
#include "univ/hash.h"

u32 NumDigits(i32 num, u32 base)
{
  u32 n = 0;
  if (num == 0) return 1;
  if (num < 0) {
    n = 1;
    num = -num;
  }
  while (num > 0) {
    num /= base;
    n++;
  }
  return n;
}

static u32 rand_state = 1;

void SeedRandom(u32 seed)
{
  rand_state = seed;
  Random();
}

u32 Random(void) {
  u32 mul = (u32)6364136223846793005u;
  u32 inc = (u32)1442695040888963407u;
  u32 x = rand_state;
  u32 r = (u32)(x >> 29);
  rand_state = x * mul + inc;
  x ^= x >> 18;
  return x >> r | x << (-r & 31);
}

i32 RandomBetween(i32 min, i32 max)
{
  u32 r, range, buckets, limit;
  if (min == max) return min;
  if (min > max) {
    u32 tmp = min;
    min = max;
    max = tmp;
  }

  range = max - min;
  buckets = MaxUInt / range;
  limit = buckets * range;

  do {
    r = Random();
  } while (r >= limit);

  return min + (i32)(r / buckets);
}
