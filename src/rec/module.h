#pragma once
#include "heap.h"
#include "seq.h"
#include "opts.h"
#include "source.h"
#include "compile.h"
#include "univ/hashmap.h"

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

ModuleResult LoadModule(char *path, Heap *mem, Val env, CassetteOpts *opts);
CompileResult LoadModules(CassetteOpts *opts, Heap *mem);
