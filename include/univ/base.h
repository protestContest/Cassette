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
typedef i32 bool;

#define true 1
#define false 0
#define ArrayCount(a)     (sizeof(a) / sizeof(a)[0])
