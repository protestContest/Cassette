#include "parse.h"
#include "parse_table.h"
#include "lex.h"
#include "univ/vec.h"
#include "univ/io.h"
#include <stdlib.h>

static ASTNode *NewNode(u32 sym, u32 num_children, ASTNode **children)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->symbol = sym;
  node->length = num_children;
  if (num_children > 0) {
    node->children = malloc(sizeof(ASTNode*)*num_children);
    for (u32 i = 0; i < num_children; i++) {
      node->children[i] = children[i];
    }
  } else {
    node->children = NULL;
  }
  return node;
}

static ASTNode *NewLeafNode(Token token)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->symbol = token.type;
  node->length = 0;
  node->token = token;
  return node;
}

static void FreeNode(ASTNode *node)
{
  if (node->children != NULL) free(node->children);
  free(node);
}

static ASTNode *Shift(Parser *p, i32 state, Token token);
static ASTNode *Reduce(Parser *p, u32 sym, u32 num, Token token);
static ASTNode *ParseNext(Parser *p, Token token);

static ASTNode *Shift(Parser *p, i32 state, Token token)
{
  VecPush(p->nodes, NewLeafNode(token));
  VecPush(p->stack, state);
  return ParseNext(p, NextToken(&p->lex));
}

static void ReduceNodes(Parser *p, u32 sym, u32 num)
{
  ASTNode *node = NewNode(sym, num, p->nodes + VecCount(p->nodes) - num);
  RewindVec(p->nodes, num);
  VecPush(p->nodes, node);
}

static ASTNode *Reduce(Parser *p, u32 sym, u32 num, Token token)
{
  Assert(VecCount(p->nodes) >= num);
  Assert(VecCount(p->stack) >= num);

  if (sym == TOP_SYMBOL) {
    ReduceNodes(p, sym, num);
    RewindVec(p->stack, num);
    return VecPop(p->nodes, NULL);
  }

  i32 next_state = actions[VecPeek(p->stack, num)][sym];
  if (next_state < 0) {
    Print("Did not expect ");
    Print((char*)symbol_names[sym]);
    Print(" from state ");
    PrintInt(VecPeek(p->stack, 0), 0);
    Print("\n");
    return NULL;
  }

  if (num == 1) {
    p->stack[VecCount(p->stack) - 1] = next_state;
  } else {
    ReduceNodes(p, sym, num);
    RewindVec(p->stack, num);
    VecPush(p->stack, next_state);
  }

  return ParseNext(p, token);
}

static ASTNode *ParseNext(Parser *p, Token token)
{
  i32 state = VecPeek(p->stack, 0);
  i32 next_state = actions[state][token.type];
  i32 reduction = reduction_syms[state];

  if (next_state >= 0) {
    return Shift(p, next_state, token);
  } else if (reduction >= 0) {
    u32 num = reduction_sizes[state];
    return Reduce(p, (u32)reduction, num, token);
  } else {
    Print("No action for ");
    Print((char*)symbol_names[token.type]);
    Print(" in state ");
    PrintInt(state, 0);
    Print("\n");
    return NULL;
  }
}

void InitParser(Parser *p, char *src)
{
  InitLexer(&p->lex, RyeToken, src);
  p->stack = NULL;
  p->nodes = NULL;
}

static ASTNode *AbstractNode(ASTNode *node);

static ASTNode *ProgramNode(ASTNode *node)
{
  Assert(node->length == 2);
  ASTNode *child = node->children[0];
  FreeNode(node);
  return AbstractNode(child);
}

static ASTNode *BinaryNode(ASTNode *node)
{
  Assert(node->length == 3);
  Assert(node->children[1]->length == 0);
  ASTNode *new_node = node->children[1];
  new_node->length = 2;
  new_node->children = node->children;
  new_node->children[0] = AbstractNode(node->children[0]);
  new_node->children[1] = AbstractNode(node->children[2]);
  node->children = NULL;
  FreeNode(node);
  return new_node;
}

static ASTNode *PassThruNode(ASTNode *node)
{
  Assert(node->length >= 1);
  ASTNode *child = node->children[0];
  FreeNode(node);
  return child;
}

static u32 NumCallArgs(ASTNode *node)
{
  u32 num = 0;
  for (u32 i = 0; i < node->length; i++) {
    if (node->children[i]->symbol == ParseSymCall) {
      num += NumCallArgs(node->children[i]);
    } else {
      num++;
    }
  }
  return num;
}

static ASTNode **AbstractCallArgs(ASTNode *node, ASTNode **children)
{
  u32 num_args = 0;
  for (u32 i = 0; i < node->length; i++) {
    if (node->children[i]->symbol == ParseSymCall) {
      AbstractCallArgs(node->children[i], children + num_args);
      num_args += node->children[i]->length;
      FreeNode(node->children[i]);
    } else {
      children[num_args] = AbstractNode(node->children[i]);
      num_args++;
    }
  }
  return children;
}

static ASTNode *CallNode(ASTNode *node)
{
  u32 num_args = NumCallArgs(node);
  ASTNode **children = malloc(sizeof(ASTNode*)*num_args);

  node->children = AbstractCallArgs(node, children);
  node->length = num_args;

  return node;
}

static ASTNode *GroupNode(ASTNode *node)
{
  Assert(node->length == 3);
  ASTNode *child = node->children[1];
  FreeNode(node);
  return AbstractNode(child);
}

static ASTNode *AbstractNode(ASTNode *node)
{
  switch (node->symbol) {
  case ParseSymProgram:
    return ProgramNode(node);
  case ParseSymSum:
  case ParseSymProduct:
    return BinaryNode(node);
  case ParseSymGroup:
    return GroupNode(node);
  case ParseSymCall:
    return CallNode(node);
  default:
    return node;
  }
}

ASTNode *Parse(char *src)
{
  Parser p;
  InitParser(&p, src);
  VecPush(p.stack, 0);
  return AbstractNode(ParseNext(&p, NextToken(&p.lex)));
}

static void PrintASTNode(ASTNode *node, u32 indent)
{
  if (node->length == 0) {
    Print("\"");
    PrintToken(node->token);
    Print("\"");
    Print("\n");
  } else {
    Print((char*)symbol_names[node->symbol]);
    Print("\n");
    for (u32 i = 0; i < node->length; i++) {
      for (u32 i = 0; i < indent+1; i++) Print(". ");
      PrintASTNode(node->children[i], indent + 1);
    }
  }
}

void PrintAST(ASTNode *ast)
{
  if (ast) PrintASTNode(ast, 0);
}
