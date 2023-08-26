#pragma once
#include "heap.h"

typedef struct {
  char *message;
  Val expr;
  u32 pos;
} CompileError;

void PrintSourceContext(char *src, u32 pos);
void PrintCompileError(CompileError *error, char *src);
