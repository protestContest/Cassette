#pragma once
#include "compile/env.h"
#include "univ/vec.h"

/* An ASTNode is a node parsed from text. */

typedef enum {
  idNode,
  constNode,
  symNode,
  strNode,
  tupleNode,
  lambdaNode,
  panicNode,
  negNode,
  notNode,
  headNode,
  tailNode,
  lenNode,
  compNode,
  eqNode,
  remNode,
  bitandNode,
  mulNode,
  addNode,
  subNode,
  divNode,
  ltNode,
  shiftNode,
  gtNode,
  joinNode,
  sliceNode,
  bitorNode,
  pairNode,
  andNode,
  orNode,
  xorNode,
  accessNode,
  callNode,
  refNode,
  ifNode,
  letNode,
  assignNode,
  doNode,
  defNode,
  importNode,
  moduleNode,
  errorNode
} NodeType;

typedef struct {
  u32 name;
  u32 value;
} NodeAttr;

typedef struct ASTNode {
  NodeType type;
  u32 start;
  u32 end;
  union {
    u32 value;
    struct ASTNode **children; /* vec */
  } data;
  NodeAttr *attrs; /* vec */
} ASTNode;
#define IsErrorNode(n)  ((n)->type == errorNode)
#define NodeCount(n)    VecCount((n)->data.children)
#define NodeChild(n,i)  ((n)->data.children[i])
#define NodeValue(n)    ((n)->data.value)
#define IsTrueNode(n)   ((n)->type == constNode && NodeValue(n) == IntVal(1))

ASTNode *NewNode(NodeType type, u32 start, u32 end, u32 value);
ASTNode *CloneNode(ASTNode *node);
void FreeNode(ASTNode *node);
void FreeNodeShallow(ASTNode *node);
bool IsTerminal(ASTNode *node);
void NodePush(ASTNode *node, ASTNode *child);
void SetNodeAttr(ASTNode *node, char *name, u32 value);
u32 GetNodeAttr(ASTNode *node, char *name);

ASTNode *SimplifyNode(ASTNode *node, Env *env);

#ifdef DEBUG
void PrintNode(ASTNode *node);
#endif
