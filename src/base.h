#pragma once

#define _GNU_SOURCE
#include <stdint.h>

#define Apple   1
#define Linux   2
#define Windows 3

#if __APPLE__
#define PLATFORM Apple
#elif __linux__
#define PLATFORM Linux
#elif _WIN32
#define PLATFORM Windows
#endif

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
typedef u32 Val;

#define true 1
#define false 0
#define ArrayCount(a)   (sizeof (a) / sizeof (a)[0])
