#pragma once

/* The parser produces an ASTNode from some input text. */

#include "node.h"
#include "lex.h"

typedef struct {
  char *text;
  Token token;
} Parser;

void InitParser(Parser *p, char *text);

ASTNode *ParseModule(char *source);
ASTNode *ParseImportList(Parser *p);
