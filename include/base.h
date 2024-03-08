#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef u32 bool;

#define true 1
#define false 0
#define ArrayCount(a)         (sizeof(a) / sizeof(a)[0])
#define Copy(from, to, size)  memcpy(to, from, size)

#define MinInt                ((i32)0x80000000)
#define MaxInt                ((i32)0x7FFFFFFF)
#define MaxUInt               ((u32)0xFFFFFFFF)
#define Abs(n)                (((n) ^ ((n) >> 31)) - ((n) >> 31))
#define Min(a, b)             ((a) ^ (((b) ^ (a)) & -((b) < (a))))
#define Max(a, b)             ((b) ^ (((b) ^ (a)) & -((b) < (a))))
#define Align(n, m)           ((((n) - 1) / (m) + 1) * (m))
#define Bit(n)                ((i64)1 << (n))
#define RightZeroBit(x)       (~(x) & ((x) + 1))

#define IsSpace(c)            ((c) == ' ' || (c) == '\t')
#define IsNewline(c)          ((c) == '\n' || (c) == '\r')
#define IsDigit(c)            ((c) >= '0' && (c) <= '9')
#define IsUppercase(c)        ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)        ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)            (IsUppercase(c) || IsLowercase(c))
#define IsHexDigit(c)         (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define HexDigit(n)           ((n) < 10 ? (n) + '0' : (n) - 10 + 'A')
#define IsPrintable(c)        ((c) >= 0x20 && (c) < 0x7F)
#define StrEq(s1, s2)         (strcmp(s1, s2) == 0)
#define MemEq(s1, s2, n)      (memcmp(s1, s2, n) == 0)
#define CopyStr(s, n)         strndup(s, n)

typedef i64 Result;
#define IsOk(res)       ((res) >= 0)
#define ResValue(res)   ((void*)(res))
#define ResError(res)   ((void*)((res) & ~Bit(63)))
#define Ok(value)       (((i64)(value)) & ~Bit(63))
#define Error(value)    (((i64)(value)) | Bit(63))
