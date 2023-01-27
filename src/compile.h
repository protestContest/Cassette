#pragma once
#include "value.h"
#include "chunk.h"

typedef struct {
  Status status;
  union {
    Chunk *chunk;
    char *error;
  };
} CompileResult;

CompileResult Compile(char *src);
