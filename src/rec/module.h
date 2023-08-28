#pragma once
#include "heap.h"
#include "seq.h"
#include "args.h"
#include "source.h"
#include "compile.h"

typedef struct {
  Val name;
  char *file;
  Seq code;
  HashMap imports;
} Module;

typedef struct ModuleResult {
  bool ok;
  union {
    Module module;
    CompileError error;
  };
} ModuleResult;

ModuleResult LoadModule(char *path, Heap *mem, Args *args);
CompileResult LoadModules(Args *args, Heap *mem);
