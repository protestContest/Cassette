#pragma once

/* A general-purpose result structure */

typedef enum {
  ok,
  parseError,
  compileError,
  moduleNotFound,
  duplicateModule,
  undefinedVariable,
  stackUnderflow,
  invalidType,
  divideByZero,
  outOfBounds,
  unhandledTrap
} StatusCode;

typedef struct {
  StatusCode status;
  union {
    void *p;
    u32 v;
  } data;
} Result;
#define IsError(result)    ((result).status > 0)

Result Ok(void *data);
Result OkVal(u32 value);
Result Error(StatusCode errorCode, char *message);
void PrintError(Result error);
