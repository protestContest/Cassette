#pragma once
#include "../value.h"
#include "parse.h"

typedef struct {
  u8 *code;
  Val *constants;
} Chunk;

Chunk *Compile(ASTNode *ast);

// #include "../value.h"
// #include "chunk.h"

// Status Compile(char *src, Chunk *chunk);
// Status CompileAST(Val *ast, Chunk *chunk);
