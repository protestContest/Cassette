#pragma once

typedef enum {
  Unreachable     = 0x00,
  Nop             = 0x01,
  Block           = 0x02,
  Loop            = 0x03,
  IfBlock         = 0x04,
  ElseBlock       = 0x05,

  EndOp           = 0x0B,
  Br              = 0x0C,
  BrIf            = 0x0D,

  CallOp          = 0x10,
  CallIndirect    = 0x11,

  DropOp          = 0x1A,

  LocalGet        = 0x20,
  LocalSet        = 0x21,
  LocalTee        = 0x22,
  GlobalGet       = 0x23,
  GlobalSet       = 0x24,
  TableGet        = 0x25,
  TableSet        = 0x26,

  I32Load         = 0x28,

  I32Store        = 0x36,

  MemSize         = 0x3F,
  MemGrow         = 0x40,
  I32Const        = 0x41,

  I32EqZ          = 0x45,
  I32Eq           = 0x46,
  I32Ne           = 0x47,
  I32LtS          = 0x48,
  I32LtU          = 0x49,
  I32GtS          = 0x4A,
  I32GtU          = 0x4B,
  I32LeS          = 0x4C,
  I32LeU          = 0x4D,
  I32GeS          = 0x4E,
  I32GeU          = 0x4F,

  I32Add          = 0x6A,
  I32Sub          = 0x6B,
  I32Mul          = 0x6C,
  I32DivS         = 0x6D,
  I32DivU         = 0x6E,
  I32RemS         = 0x6F,
  I32RemU         = 0x70,
  I32And          = 0x71,
  I32Or           = 0x72,
  I32Xor          = 0x73
} OpCode;

#define EmptyBlock 0x40

u32 IntSize(i32 n);
void PushByte(u8 **code, u8 n);
void PushInt(u8 **code, i32 n);
void AppendBytes(u8 **a, u8 **b);
