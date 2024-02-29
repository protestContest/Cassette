#pragma once

typedef enum {
  EndOp           = 0x0B,
  LocalGet        = 0x20,
  I32Const        = 0x41,
  F32Const        = 0x43,
  I32Add          = 0x6A,
  I32Sub          = 0x6B,
  I32Mul          = 0x6C,
  I32DivS         = 0x6D,
  I32DivU         = 0x6E,
  I32RemS         = 0x6F,
  I32RemU         = 0x70,
  I64Add          = 0x7C,
  I64Sub          = 0x7D,
  I64Mul          = 0x7E,
  I64DivS         = 0x7F,
  I64DivU         = 0x80,
  I64RemS         = 0x81,
  I64RemU         = 0x82,
  F32Add          = 0x92,
  F32Sub          = 0x93,
  F32Mul          = 0x94,
  F32Div          = 0x95,
  F32ConvertI32S  = 0xB2
} OpCode;
