#pragma once

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned long u32;
typedef signed long i32;
typedef unsigned long long u64;
typedef signed long long i64;
typedef float f32;
typedef double f64;
typedef _Bool bool;
#define true (bool)1
#define false (bool)0
#define NULL (void*)0
#define ArrayCount(a)   (sizeof (a) / sizeof (a)[0])
