#pragma once

typedef enum {
  LocalGet = 0x20,
  I32Const = 0x41,
  F32Const = 0x43,
  I32Add = 0x6A,
  I32Sub = 0x6B,
  I32Mul = 0x6C,
  I32DivS = 0x6D,
  I32DivU = 0x6E,
  I32RemS = 0x6F,
  I32RemU = 0x70
} OpCode;
