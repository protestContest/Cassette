#include "univ/math.h"
#include "univ/bitstream.h"
#include "univ/str.h"

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

static u32 randSeed = 1;

void SeedRandom(u32 seed)
{
  randSeed = seed;
  Random();
}

u32 Random(void)
{
  u32 hiSeed = randSeed >> 16;
  u32 loSeed = randSeed & 0xFFFF;
  u32 a, c;

  a = 0x41A7 * loSeed;
  c = 0x41A7 * hiSeed + (a >> 16);
  c = ((c & 0x7FFF) << 16) + ((2*c) >> 16);
  a = ((a & 0xFFFF) - 0x7FFFFFFF) + c;

  if ((i32)a < 0) a += 0x7FFFFFFF;
  randSeed = a;
  return randSeed;
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

static char base32encode[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
  'G', 'H', 'J', 'K', 'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'W', 'X', 'Y', 'Z'
};
static char base32decode[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  0, 0, 0, 0, 0, 0, 0, 10, 11, 12,
  13, 14, 15, 16, 17, 0, 18, 19, 0, 20,
  21, 0, 22, 23, 24, 25, 26, 0, 27, 28,
  29, 30, 31
};

char *Base32Encode(void *data, u32 len)
{
  u8 *bytes = (u8*)data;
  u32 i = 0;
  u32 dst_len = Align(len*8, 5) / 5;
  u8 *dst = malloc(dst_len+1);
  BitStream stream;
  InitBitStream(&stream, bytes, len, true);
  while (HasBits(&stream)) {
    u8 bits = ReadBits(&stream, 5);
    dst[i++] = base32encode[bits];
  }
  dst[dst_len] = 0;
  return (char*)dst;
}

char *Base32Decode(void *data, u32 len)
{
  u32 i;
  u32 nBits = 5;
  BitStream stream;
  u8 *bytes = (u8*)data;
  InitBitStream(&stream, 0, 0, true);
  for (i = 0; i < len; i++) {
    char c = UpChar(bytes[i]);
    switch (c) {
    case 'O':
      c = '0';
      break;
    case 'I':
    case 'L':
      c = '1';
      break;
    }
    WriteBits(&stream, base32decode[c - '0'], nBits);
  }
  FinalizeBits(&stream);
  return (char*)stream.data;
}
