#pragma once
#include "mem.h"
#include "lex.h"
#include "symbols.h"

typedef struct {
  Lexer lex;
  SymbolTable symbols;
  Mem mem;
} Parser;

void InitParser(Parser *p);
void DestroyParser(Parser *p);

Val Parse(char *source, Parser *p);

void PrintAST(Val ast, u32 level, Mem *mem, SymbolTable *symbols);
void PrintParseError(Parser *p);
