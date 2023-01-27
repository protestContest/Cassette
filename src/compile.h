#pragma once
#include "value.h"
#include "reader.h"
#include "chunk.h"

typedef struct {
  Reader r;
  Chunk *chunk;
  Token current;
} Parser;

Status Compile(char *src, Chunk *chunk);
