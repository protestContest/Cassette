#pragma once
#include "heap.h"
#include "seq.h"

typedef struct {
  bool ok;
  union {
    Seq result;
    char *error;
  };
} CompileResult;

CompileResult Compile(Val ast, Val env, Heap *mem);
