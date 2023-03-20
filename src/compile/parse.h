#pragma once
#include "lex.h"

typedef enum {
  SymEnd,
  SymError,
  SymID,
  SymNum,
  SymArrow,
  SymLParen,
  SymRParen,
  SymMinus,
  SymPlus,
  SymStar,
  SymSlash,
  SymAccept,
  SymExpr,
  SymLambda,
  SymCall,
  SymIDs,
  SymArgs,
  SymGroup,
  SymSum,
  SymProduct,
  SymNegative,
} ParseSymbol;
#define NUM_PARSE_SYM (SymNegative + 1)

typedef struct {
  ParseSymbol sym;
  Token token;
  u32 *children;
} ASTNode;

typedef struct {
  ASTNode *nodes;
  u32 root;
} AST;

typedef struct {
  Lexer lex;
  Token next_token;
  u32 *stack;
  u32 *symbols;
  ASTNode *nodes;
} Parser;

AST Parse(char *src);
void PrintAST(AST ast);
