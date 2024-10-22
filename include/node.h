#pragma once
#include "univ/vec.h"

/* An ASTNode is a node parsed from text. */

typedef enum {
  idNode, constNode, symNode, strNode, tupleNode, lambdaNode, panicNode, negNode,
  notNode, headNode, tailNode, lenNode, compNode, eqNode, remNode, bitandNode,
  mulNode, addNode, subNode, divNode, ltNode, shiftNode, gtNode, joinNode,
  sliceNode, bitorNode, pairNode, andNode, orNode, accessNode, callNode,
  trapNode, refNode, ifNode, letNode, assignNode,  doNode, defNode, importNode,
  moduleNode, errorNode
} NodeType;

typedef struct ASTNode {
  NodeType type;
  u32 start;
  u32 end;
  union {
    u32 value;
    struct ASTNode **children; /* vec */
  } data;
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
void PrintNode(ASTNode *node);
char *NodeTypeName(i32 type);
