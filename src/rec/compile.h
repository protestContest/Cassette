#pragma once
#include "heap.h"
#include "seq.h"
#include "source.h"
#include "args.h"

typedef struct {
  bool ok;
  union {
    Seq code;
    CompileError error;
  };
} CompileResult;

struct ModuleResult Compile(Val ast, Args *args, Heap *mem);
