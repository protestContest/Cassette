#pragma once
#include "chunk.h"
#include "lex.h"
#include "assemble.h"
#include "module.h"

typedef struct {
  bool ok;
  Val node;
  char *message;
  Seq result;
} CompileResult;

CompileResult Compile(Val ast, Mem *mem);
