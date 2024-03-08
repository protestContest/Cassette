#include "wasm.h"
#include <univ.h>

u32 IntSize(i32 n)
{
  if (Abs(n) < Bit(6)) return 1;
  if (Abs(n) < Bit(12)) return 2;
  if (Abs(n) < Bit(18)) return 3;
  if (Abs(n) < Bit(24)) return 4;
  if (Abs(n) < Bit(30)) return 5;
  return 6;
}

void PushByte(u8 **bytes, u8 n)
{
  VecPush(*bytes, n);
}

void PushInt(u8 **bytes, i32 n)
{
  u32 i;
  u8 byte;
  u32 size = IntSize(n);
  for (i = 0; i < size - 1; i++) {
    byte = ((n >> (7*i)) & 0x7F) | 0x80;
    PushByte(bytes, byte);
  }
  byte = ((n >> (7*(size-1))) & 0x7F);
  PushByte(bytes, byte);
}

void AppendBytes(u8 **a, u8 **b)
{
  u32 a_count = VecCount(*a);
  GrowVec(*a, VecCount(*b));
  Copy(*b, *a + a_count, VecCount(*b));
}

