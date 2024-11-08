#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef _Bool
#define _Bool int
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
typedef i32 bool;

#define true 1
#define false 0
#define ArrayCount(a)     (sizeof(a) / sizeof(a)[0])
#define MinInt            ((i32)0x80000000)
#define MaxInt            ((i32)0x7FFFFFFF)
#define MaxUInt           ((u32)0xFFFFFFFF)
#define Min(a, b)         ((a) > (b) ? (b) : (a))
#define Max(a, b)         ((a) > (b) ? (a) : (b))
