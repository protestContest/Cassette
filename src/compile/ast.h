#pragma once
#include "../value.h"
#include "scan.h"

typedef struct ASTNode {
  Token token;
  Val value;
  struct ASTNode *children;
} ASTNode;

ASTNode MakeNode(Token token, Val value);
ASTNode PushNode(ASTNode node, ASTNode child);
void PrintAST(ASTNode ast);
