#pragma once
#include "chunk.h"
#include "lex.h"

typedef struct Compiler Compiler;

// void InitCompiler(Compiler *compiler, char *source);
// void CompileExpr(Compiler *compiler, u32 precedence);
Chunk Compile(char *source);
