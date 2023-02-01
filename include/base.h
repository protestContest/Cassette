#pragma once

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef unsigned char byte;

typedef enum {
  Ok,
  Error,
  Unknown,
} Status;

#define Bit(n)    (1 << (n))

#define ArrayCount(a)   (sizeof (a) / sizeof (a)[0])

#define PrintInto(str, fmt, ...)                                  \
  do {                                                            \
    u32 size = snprintf(NULL, 0, fmt __VA_OPT__(,) __VA_ARGS__);  \
    *((void **)&(str)) = realloc(str, size);                      \
    sprintf(str, fmt __VA_OPT__(,) __VA_ARGS__);                  \
  } while (0)


#define Fatal(...)  do {        \
  fflush(stdout);               \
  fprintf(stderr, "Fatal: ");   \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n");        \
  fflush(stderr);               \
  exit(1);                      \
} while (0)

#define DEBUG 1
#define READ  0
#define EVAL  1

#define Debug(level, ...)  do {   \
  if (DEBUG && level) {           \
    fprintf(stderr, "; ");        \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
    fflush(stderr);               \
  }                               \
} while (0)
