#pragma once
#include "heap.h"
#include "seq.h"
#include "source.h"

typedef struct {
  bool ok;
  union {
    Seq code;
    CompileError error;
  };
} CompileResult;

struct ModuleResult Compile(Val ast, Val env, Heap *mem);
