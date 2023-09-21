#pragma once
#include "heap.h"
#include "seq.h"
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

ModuleResult LoadModule(char *file, Heap *mem, Val env);
CompileResult LoadModules(char *entry_file, char *mod_path, Heap *mem);
