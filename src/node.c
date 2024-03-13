#include "node.h"
#include <univ.h>

Node *MakeTerminal(NodeType type, u32 position, u32 value)
{
  Node *node = malloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  node->children = 0;
  node->value = value;
  return node;
}

Node *MakeNode(NodeType type, u32 pos)
{
  Node *node = malloc(sizeof(Node));
  node->type = type;
  node->pos = pos;
  node->children = 0;
  node->value = 0;
  return node;
}

bool IsTerminal(Node *node)
{
  switch (node->type) {
  case nilNode:
  case idNode:
  case intNode:
  case floatNode:
  case stringNode:
    return true;
  default:
    return false;
  }
}

void FreeNode(Node *node)
{
  u32 i;
  for (i = 0; i < VecCount(node->children); i++) FreeNode(node->children[i]);
  FreeVec(node->children);
  free(node);
}

char *NodeName(Node *node)
{
  switch (node->type) {
  case nilNode:     return "nil";
  case idNode:      return "id";
  case intNode:     return "int";
  case floatNode:   return "float";
  case stringNode:  return "string";
  case listNode:    return "list";
  case tupleNode:   return "tuple";
  case doNode:      return "do";
  case ifNode:      return "if";
  case accessNode:  return "access";
  case callNode:    return "call";
  case negNode:     return "neg";
  case notNode:     return "not";
  case bitNotNode:  return "bit not";
  case lenNode:     return "len";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case bitAndNode:  return "bit and";
  case addNode:     return "add";
  case subNode:     return "sub";
  case bitOrNode:   return "bit or";
  case shiftNode:   return "shift";
  case ltNode:      return "lt";
  case gtNode:      return "gt";
  case splitNode:   return "split";
  case tailNode:    return "tail";
  case joinNode:    return "join";
  case pairNode:    return "pair";
  case eqNode:      return "eq";
  case andNode:     return "and";
  case orNode:      return "or";
  case lambdaNode:  return "lambda";
  case defNode:     return "def";
  case letNode:     return "let";
  case importNode:  return "import";
  case moduleNode:  return "module";
  default:          return "?";
  }
}

static void Indent(u32 level, u32 lines)
{
  u32 i;
  for (i = 0; i < level; i++) {
    if (lines & Bit(i)) printf("│ ");
    else printf("  ");
  }
}

static void PrintASTNode(Node *node, u32 level, u32 lines);

static void PrintChildNode(Node *node, u32 level, u32 lines, bool last)
{
  if (last) {
    Indent(level, lines);
    printf("└╴");
  } else {
    lines |= Bit(level);
    Indent(level, lines);
    printf("├╴");
  }
  PrintASTNode(node, level+1, lines);
}

static void PrintLastChildNode(Node *node, u32 level, u32 lines)
{
  Indent(level, lines);
  printf("└╴");
}

static void PrintChildNodes(Node **nodes, u32 level, u32 lines, bool last)
{
  u32 i;
  for (i = 0; i < VecCount(nodes); i++) {
    PrintChildNode(nodes[i], level, lines, last && i == VecCount(nodes)-1);
  }
}

static void PrintASTNode(Node *node, u32 level, u32 lines)
{
  char *name = NodeName(node);
  u32 i;

  printf("%s:%d", name, node->pos);

  switch (node->type) {
  case nilNode:
    printf("\n");
    break;
  case idNode:
    printf(" %s\n", SymbolName(node->value));
    break;
  case intNode:
    printf(" %d\n", node->value);
    break;
  case floatNode:
    /*TerminalNode *num = (TerminalNode*)node;*/
    printf(" <float>\n");
    break;
  case stringNode:
    printf(" \"%s\"\n", SymbolName(node->value));
    break;
  case listNode:
  case tupleNode:
    printf("\n");
    PrintChildNodes(node->children, level, lines, true);
    break;
  case doNode: {
    Node *locals = DoNodeLocals(node);
    Node *stmts = DoNodeStmts(node);
    printf(" Locals: [");
    for (i = 0; i < VecCount(locals->children); i++) {
      char *assign = SymbolName(locals->children[i]->value);
      printf("%s", assign);
      if (i < VecCount(locals->children) - 1) printf(", ");
    }
    printf("]\n");
    PrintChildNodes(stmts->children, level, lines, true);
    break;
  }
  case ifNode:
    printf("\n");
    PrintChildNode(IfNodePredicate(node), level, lines, false);
    PrintChildNode(IfNodeConsequent(node), level, lines, false);
    PrintChildNode(IfNodeAlternative(node), level, lines, true);
    break;
  case accessNode:
  case callNode:
  case negNode:
  case notNode:
  case bitNotNode:
  case lenNode:
  case mulNode:
  case divNode:
  case remNode:
  case bitAndNode:
  case addNode:
  case subNode:
  case bitOrNode:
  case shiftNode:
  case ltNode:
  case gtNode:
  case splitNode:
  case tailNode:
  case joinNode:
  case pairNode:
  case eqNode:
  case andNode:
  case orNode:
    printf("\n");
    PrintChildNodes(node->children, level, lines, true);
    break;
  case lambdaNode: {
    Node *params = LambdaNodeParams(node);
    Node *body = LambdaNodeBody(node);
    printf(" (");
    for (i = 0; i < VecCount(params->children); i++) {
      printf("%s", SymbolName(params->children[i]->value));
      if (i < VecCount(params->children) - 1) printf(", ");
    }
    printf(")\n");
    PrintChildNodes(body->children, level, lines, true);
    break;
  }
  case letNode:
  case defNode:
    printf(" %s\n", SymbolName(AssignNodeVar(node)->value));
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(AssignNodeValue(node), level+1, lines);
    break;
  case importNode:
    printf(" %s as %s\n", SymbolName(ImportNodeMod(node)->value),
                          SymbolName(ImportNodeAlias(node)->value));
    break;
  case moduleNode:
    printf(" %s\n", SymbolName(ModuleNodeName(node)->value));
    PrintChildNodes(ModuleNodeImports(node)->children, level, lines, false);
    PrintChildNode(ModuleNodeBody(node), level, lines, true);
    break;
  }
}

void PrintAST(Node *ast)
{
  PrintASTNode(ast, 0, 0);
}
