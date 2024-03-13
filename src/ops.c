#include "ops.h"
#include <univ.h>

u32 IntSize(i32 n)
{
  u32 size = 1;
  while (n >= Bit(6) || n < -Bit(6)) {
    size++;
    n >>= 7;
  }
  return size;
}

void EmitByte(u8 n, u8 **code)
{
  VecPush(*code, n);
}

void EmitInt(i32 n, u8 **code)
{
  u32 i;
  u8 byte;
  u32 size = IntSize(n);

  if (n == (i32)AddSymbol("false")) {
    printf("x");
  }

  for (i = 0; i < size - 1; i++) {
    byte = ((n >> (7*i)) & 0x7F) | 0x80;
    EmitByte(byte, code);
  }
  byte = ((n >> (7*(size-1))) & 0x7F);
  EmitByte(byte, code);
}

void AppendBytes(u8 **a, u8 **b)
{
  u32 a_count = VecCount(*a);
  GrowVec(*a, VecCount(*b));
  Copy(*b, *a + a_count, VecCount(*b));
}

void EmitConst(i32 n, u8 **code)
{
  EmitByte(I32Const, code);
  EmitInt(n, code);
}

void EmitLoad(i32 offset, u8 **code)
{
  EmitByte(I32Load, code);
  EmitInt(2, code);
  EmitInt(offset, code);
}

void EmitLoadByte(i32 offset, u8 **code)
{
  EmitByte(I32Load8, code);
  EmitInt(0, code);
  EmitInt(offset, code);
}

void EmitStore(i32 offset, u8 **code)
{
  EmitByte(I32Store, code);
  EmitInt(2, code);
  EmitInt(offset, code);
}

void EmitStoreByte(i32 offset, u8 **code)
{
  EmitByte(I32Store8, code);
  EmitInt(0, code);
  EmitInt(offset, code);
}

void EmitGetGlobal(i32 idx, u8 **code)
{
  EmitByte(GlobalGet, code);
  EmitByte(idx, code);
}

void EmitSetGlobal(i32 idx, u8 **code)
{
  EmitByte(GlobalSet, code);
  EmitInt(idx, code);
}

void EmitGetLocal(i32 idx, u8 **code)
{
  EmitByte(LocalGet, code);
  EmitInt(idx, code);
}

void EmitSetLocal(i32 idx, u8 **code)
{
  EmitByte(LocalSet, code);
  EmitInt(idx, code);
}

void EmitTeeLocal(i32 idx, u8 **code)
{
  EmitByte(LocalTee, code);
  EmitInt(idx, code);
}

void EmitCall(i32 funcidx, u8 **code)
{
  EmitByte(Call, code);
  EmitInt(funcidx, code);
}

void EmitCallIndirect(i32 typeidx, u8 **code)
{
  EmitByte(CallIndirect, code);
  EmitInt(typeidx, code);
  EmitInt(0, code);
}
