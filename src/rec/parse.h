#pragma once
#include "heap.h"
#include "lex.h"

Val Parse(char *source, Heap *mem);
Val ParseStmt(Lexer *lex, Heap *mem);
