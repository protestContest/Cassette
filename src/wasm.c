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

void EmitByte(u8 n, u8 **bytes)
{
  VecPush(*bytes, n);
}

void EmitInt(i32 n, u8 **bytes)
{
  u32 i;
  u8 byte;
  u32 size = IntSize(n);
  for (i = 0; i < size - 1; i++) {
    byte = ((n >> (7*i)) & 0x7F) | 0x80;
    EmitByte(byte, bytes);
  }
  byte = ((n >> (7*(size-1))) & 0x7F);
  EmitByte(byte, bytes);
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

void EmitStore(i32 offset, u8 **code)
{
  EmitByte(I32Store, code);
  EmitInt(2, code);
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
  EmitByte(CallOp, code);
  EmitInt(funcidx, code);
}

void EmitCallIndirect(i32 typeidx, u8 **code)
{
  EmitByte(CallIndirect, code);
  EmitInt(typeidx, code);
  EmitInt(0, code);
}

