#include "ast.h"
#include "../runtime/print.h"
#include <univ/vec.h>
#include <univ/io.h>

ASTNode MakeNode(Token token, Val value)
{
  ASTNode node;
  node.token = token;
  node.value = value;
  node.children = NULL;
  return node;
}

ASTNode PushNode(ASTNode node, ASTNode child)
{
  VecPush(node.children, child);
  return node;
}

static void PrintASTLevel(ASTNode ast, u32 indent)
{
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print(TokenStr(ast.token.type));

  if (ast.children) {
    Print("\n");
    for (u32 i = 0; i < indent; i++) Print("  ");
    for (u32 i = 0; i < VecCount(ast.children); i++) {
      PrintASTLevel(ast.children[i], indent + 1);
    }
  } else {
    for (u32 i = 0; i < indent; i++) Print("  ");
    PrintVal(NULL, nil, ast.value);
    Print("\n");
  }
}

void PrintAST(ASTNode ast)
{
  PrintASTLevel(ast, 0);
}
