#pragma once
#include "module.h"

typedef struct CompileResult {
  bool ok;
  i32 pos;
  char *error;
} CompileResult;

CompileResult Compile(i32 ast, Module *mod);
