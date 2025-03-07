#pragma once

/* The parser produces an ASTNode from some input text. */

#include "compile/node.h"
#include "compile/lex.h"

typedef struct {
  char *text; /* borrowed */
  Token token;
} Parser;

void InitParser(Parser *p, char *text);

ASTNode *ParseModule(char *source);
ASTNode *ParseModuleHeader(char *source);
ASTNode *ParseImportList(Parser *p);
