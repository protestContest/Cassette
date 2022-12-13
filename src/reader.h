#pragma once

#include "mem.h"

typedef struct {
  Value tokens;
  char *src;
} TokenList;

TokenList Read(void);
Value Parse(char *src);

void DumpAST(TokenList tokens);
