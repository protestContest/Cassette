#pragma once

#include <stdint.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef unsigned char byte;

#define NULL 0
#define bool _Bool
#define true 1
#define false 0

typedef enum {
  Ok,
  Error,
  Unknown,
} Status;

#define Bit(n)    (1 << (n))
#define ArrayCount(a)   (sizeof (a) / sizeof (a)[0])

#define Assert(c) if (!(c)) __builtin_trap()
