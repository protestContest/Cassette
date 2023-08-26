#pragma once
#include "heap.h"
#include "seq.h"
#include "args.h"
#include "source.h"

typedef struct {
  Val name;
  Seq code;
  HashMap imports;
} Module;

typedef struct {
  bool ok;
  union {
    Module module;
    CompileError error;
  };
} ModuleResult;

ModuleResult LoadModule(char *path, Heap *mem, Args *args);
ModuleResult LoadModules(Args *args, Heap *mem);
