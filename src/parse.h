#pragma once
#include "lex.h"

typedef struct ASTNode {
  u32 symbol;
  u32 length;
  union {
    Token token;
    struct ASTNode **children;
  };
} ASTNode;

typedef struct {
  i32 id;
  ASTNode value;
} ParseState;

ASTNode *Parse(char *src);
void PrintAST(ASTNode *ast);
