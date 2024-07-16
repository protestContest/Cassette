#include "leb.h"

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
