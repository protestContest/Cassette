#include "univ/math.h"

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

u32 PopCount(u32 n)
{
  n = n - ((n >> 1) & 0x55555555);
  n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
  n = (n + (n >> 4)) & 0x0F0F0F0F;
  n = n + (n >> 8);
  n = n + (n >> 16);
  return n & 0x0000003F;
}

i32 LEBSize(i32 num)
{
  i32 size = 0;
  while (num > 0x3F || num < -64) {
    size++;
    num >>= 7;
  }
  return size + 1;
}

void WriteLEB(i32 num, u32 pos, u8 *buf)
{
  while (num > 0x3F || num < -64) {
    u8 byte = ((num & 0x7F) | 0x80) & 0xFF;
    buf[pos++] = byte;
    num >>= 7;
  }
  buf[pos++] = num & 0x7F;
}

i32 ReadLEB(u32 index, u8 *buf)
{
  i8 byte = buf[index++];
  i32 num = byte & 0x7F;
  u32 shift = 7;
  while (byte & 0x80) {
    byte = buf[index++];
    num |= (byte & 0x7F) << shift;
    shift += 7;
  }
  if (shift < 8*sizeof(i32) && (num & 1<<(shift-1))) {
    num |= ~0 << shift;
  }
  return num;
}

u32 Hash(void *data, u32 size)
{
  return AppendHash(EmptyHash, data, size);
}

u32 AppendHash(u32 hash, void *data, u32 size)
{
  u32 i;
  u8 *bytes = (u8*)data;
  for (i = 0; i < size; i++) {
    hash = ((hash << 5) + hash) + bytes[i];
  }
  return hash;
}

u32 FoldHash(u32 hash, i32 size)
{
  u32 mask = 0xFFFFFFFF >> (32 - size);
  return (hash ^ ((hash & ~mask) >> size)) & mask;
}

u32 ByteSwap(u32 n)
{
  return
    (n << 24) |
    ((n&0x0000FF00) << 8) |
    ((n&0x00FF0000) >> 8) |
    (n >> 24);
}

static u32 *crcTable = 0;

static void GenerateCRCTable(void)
{
  u32 c;
  u16 n, k;
  crcTable = malloc(sizeof(u32)*256);

  for (n = 0; n < 256; n++) {
    c = (u32)n;
    for (k = 0; k < 8; k++) {
      if (c & 1) {
        c = 0XEDB88320 ^ (c >> 1);
      } else {
        c = c >> 1;
      }
    }
    crcTable[n] = c;
  }
}

u32 CRC32(u8 *data, u32 size)
{
  u32 result = 0xFFFFFFFF;
  u32 i;

  if (!crcTable) GenerateCRCTable();

  for (i = 0; i < size; i++) {
    u32 index = (result ^ data[i]) & 0xff;
    result = (result >> 8) ^ crcTable[index];
  }

  result ^= 0xFFFFFFFF;
  return result;
}
