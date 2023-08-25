#pragma once
#include "heap.h"
#include "seq.h"

typedef struct {
  Val name;
  Seq code;
  HashMap imports;
} Module;

typedef struct {
  bool ok;
  union {
    Module module;
    char *error;
  };
} ModuleResult;

ModuleResult LoadModule(char *path, Heap *mem);
ModuleResult LoadProject(char *source_folder, char *entry_file, Heap *mem);
