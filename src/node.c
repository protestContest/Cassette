#include "node.h"
#include <univ.h>

TerminalNode *MakeTerminal(NodeType type, u32 position, u32 value)
{
  TerminalNode *node = malloc(sizeof(TerminalNode));
  node->type = type;
  node->pos = position;
  node->value = value;
  return node;
}

Node *MakeNode(NodeType type, u32 pos)
{
  Node *node;
  switch (type) {
  case listNode:
  case tupleNode:
    node = malloc(sizeof(ListNode));
    ((ListNode*)node)->items = 0;
    break;
  case negNode:
  case lenNode:
  case notNode:
    node = malloc(sizeof(UnaryNode));
    ((UnaryNode*)node)->child = 0;
    break;
  case pairNode:
  case accessNode:
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case ltEqNode:
  case inNode:
  case eqNode:
  case notEqNode:
  case gtNode:
  case gtEqNode:
  case andNode:
  case orNode:
    node = malloc(sizeof(BinaryNode));
    ((BinaryNode*)node)->left = 0;
    ((BinaryNode*)node)->right = 0;
    break;
  case ifNode:
    node = malloc(sizeof(IfNode));
    ((IfNode*)node)->predicate = 0;
    ((IfNode*)node)->ifTrue = 0;
    ((IfNode*)node)->ifFalse = 0;
    break;
  case doNode:
    node = malloc(sizeof(DoNode));
    ((DoNode*)node)->locals = 0;
    ((DoNode*)node)->stmts = 0;
    break;
  case callNode:
    node = malloc(sizeof(CallNode));
    ((CallNode*)node)->op = 0;
    ((CallNode*)node)->args = 0;
    break;
  case lambdaNode:
    node = malloc(sizeof(LambdaNode));
    ((LambdaNode*)node)->params = 0;
    ((LambdaNode*)node)->body = 0;
    break;
  case defNode:
  case letNode:
    node = malloc(sizeof(LetNode));
    ((LetNode*)node)->var = 0;
    ((LetNode*)node)->value = 0;
    break;
  case importNode:
    node = malloc(sizeof(ImportNode));
    ((ImportNode*)node)->mod = 0;
    ((ImportNode*)node)->alias = 0;
    break;
  case moduleNode:
    node = malloc(sizeof(ModuleNode));
    ((ModuleNode*)node)->name = 0;
    ((ModuleNode*)node)->filename = 0;
    ((ModuleNode*)node)->imports = 0;
    ((ModuleNode*)node)->body = 0;
    break;
  default:
    return (Node*)MakeTerminal(type, pos, 0);
  }

  node->type = type;
  node->pos = pos;
  return node;
}

bool IsTerminal(Node *node)
{
  switch (node->type) {
  case nilNode:
  case varNode:
  case intNode:
  case floatNode:
  case stringNode:
  case symbolNode:
    return true;
  default:
    return false;
  }
}

void FreeNode(Node *node)
{
  u32 i;
  Node **children;

  if (!node) return;

  switch (node->type) {
  case listNode:
  case tupleNode:
    children = ((ListNode*)node)->items;
    for (i = 0; i < VecCount(children); i++) FreeNode(children[i]);
    FreeVec(children);
    break;
  case negNode:
  case lenNode:
  case notNode:
    FreeNode(((UnaryNode*)node)->child);
    break;
  case pairNode:
  case accessNode:
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case ltEqNode:
  case inNode:
  case eqNode:
  case notEqNode:
  case gtNode:
  case gtEqNode:
  case andNode:
  case orNode:
    FreeNode(((BinaryNode*)node)->left);
    FreeNode(((BinaryNode*)node)->right);
    break;
  case ifNode:
    FreeNode(((IfNode*)node)->predicate);
    FreeNode(((IfNode*)node)->ifTrue);
    FreeNode(((IfNode*)node)->ifFalse);
    break;
  case doNode:
    children = ((DoNode*)node)->stmts;
    for (i = 0; i < VecCount(children); i++) FreeNode(children[i]);
    FreeVec(children);
    FreeVec(((DoNode*)node)->locals);
    break;
  case callNode:
    children = ((CallNode*)node)->args;
    for (i = 0; i < VecCount(children); i++) FreeNode(children[i]);
    FreeVec(children);
    FreeNode(((CallNode*)node)->op);
    break;
  case lambdaNode:
    FreeVec(((LambdaNode*)node)->params);
    FreeNode(((LambdaNode*)node)->body);
    break;
  case defNode:
  case letNode:
    FreeNode(((LetNode*)node)->value);
    break;
  case moduleNode:
    children = (Node**)((ModuleNode*)node)->imports;
    for (i = 0; i < VecCount(children); i++) FreeNode(children[i]);
    FreeVec(children);
    FreeNode((Node*)((ModuleNode*)node)->body);
    break;
  default:
    break;
  }

  free(node);
}

char *NodeName(Node *node)
{
  switch (node->type) {
  case nilNode:     return "nil";
  case varNode:      return "var";
  case intNode:     return "int";
  case floatNode:   return "float";
  case stringNode:  return "string";
  case symbolNode:  return "symbol";
  case listNode:    return "list";
  case tupleNode:   return "tuple";
  case pairNode:    return "pair";
  case accessNode:  return "access";
  case negNode:     return "neg";
  case addNode:     return "add";
  case subNode:     return "sub";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case ltNode:      return "lt";
  case ltEqNode:    return "ltEq";
  case inNode:      return "in";
  case lenNode:     return "len";
  case eqNode:      return "eq";
  case notEqNode:   return "notEq";
  case gtNode:      return "gt";
  case gtEqNode:    return "gtEq";
  case notNode:     return "not";
  case andNode:     return "and";
  case orNode:      return "or";
  case ifNode:      return "if";
  case doNode:      return "do";
  case callNode:    return "call";
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
  case varNode: {
    TerminalNode *var = (TerminalNode*)node;
    printf(" %s\n", SymbolName(var->value));
    break;
  }
  case intNode: {
    TerminalNode *num = (TerminalNode*)node;
    printf(" %d\n", num->value);
    break;
  }
  case floatNode: {
    /*TerminalNode *num = (TerminalNode*)node;*/
    printf(" <float>\n");
    break;
  }
  case stringNode: {
    TerminalNode *str = (TerminalNode*)node;
    printf(" \"%s\"\n", SymbolName(str->value));
    break;
  }
  case symbolNode: {
    TerminalNode *sym = (TerminalNode*)node;
    printf(" :%s\n", SymbolName(sym->value));
    break;
  }
  case listNode:
  case tupleNode: {
    ListNode *list = (ListNode*)node;
    printf("\n");
    PrintChildNodes(list->items, level, lines, true);
    break;
  }
  case negNode:
  case lenNode:
  case notNode: {
    UnaryNode *unnode = (UnaryNode*)node;
    printf("\n");
    PrintChildNode(unnode->child, level, lines, true);
    break;
  }
  case pairNode:
  case accessNode:
  case addNode:
  case subNode:
  case mulNode:
  case divNode:
  case remNode:
  case ltNode:
  case ltEqNode:
  case inNode:
  case eqNode:
  case notEqNode:
  case gtNode:
  case gtEqNode:
  case andNode:
  case orNode: {
    BinaryNode *binnode = (BinaryNode*)node;
    printf("\n");
    PrintChildNode(binnode->left, level, lines, false);
    PrintChildNode(binnode->right, level, lines, true);
    break;
  }
  case ifNode: {
    IfNode *ifnode = (IfNode*)node;
    printf("\n");
    PrintChildNode(ifnode->predicate, level, lines, false);
    PrintChildNode(ifnode->ifTrue, level, lines, false);
    PrintChildNode(ifnode->ifFalse, level, lines, true);
    break;
  }
  case doNode: {
    DoNode *donode = (DoNode*)node;
    printf(" Locals: [");
    for (i = 0; i < VecCount(donode->locals); i++) {
      char *assign = SymbolName(donode->locals[i]);
      printf("%s", assign);
      if (i < VecCount(donode->locals) - 1) printf(", ");
    }
    printf("]\n");
    PrintChildNodes(donode->stmts, level, lines, true);
    break;
  }
  case callNode: {
    CallNode *call = (CallNode*)node;
    printf("\n");
    PrintChildNode(call->op, level, lines, false);
    PrintChildNodes(call->args, level, lines, true);
    break;
  }
  case lambdaNode: {
    LambdaNode *lambda = (LambdaNode*)node;
    printf(" (");
    for (i = 0; i < VecCount(lambda->params); i++) {
      printf("%s", SymbolName(lambda->params[i]));
      if (i < VecCount(lambda->params) - 1) printf(", ");
    }
    printf(")\n");
    PrintChildNode(lambda->body, level, lines, true);
    break;
  }
  case letNode:
  case defNode: {
    LetNode *let = (LetNode*)node;
    printf(" %s\n", SymbolName(let->var));
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(let->value, level+1, lines);
    break;
  }
  case importNode: {
    ImportNode *import = (ImportNode*)node;
    printf(" %s as %s\n", SymbolName(import->mod), SymbolName(import->alias));
    break;
  }
  case moduleNode: {
    ModuleNode *mod = (ModuleNode*)node;
    char *name = SymbolName(mod->name);
    printf(" %s\n", name);
    PrintChildNodes((Node**)mod->imports, level, lines, false);
    PrintChildNode((Node*)mod->body, level, lines, true);
    break;
  }
  }
}

void PrintAST(Node *ast)
{
  PrintASTNode(ast, 0, 0);
}
