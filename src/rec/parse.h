#pragma once
#include "heap.h"
#include "lex.h"
#include "source.h"

typedef struct {
  bool ok;
  union {
    Val value;
    CompileError error;
  };
} ParseResult;

ParseResult Parse(char *source, Heap *mem);
