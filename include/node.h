#pragma once

/* An ASTNode is a node parsed from text. */

typedef enum {
  listNode, constNode, idNode, strNode, tupleNode, negNode, notNode, headNode, tailNode,
  lenNode, accessNode, compNode, eqNode, remNode, bitandNode, mulNode, addNode,
  subNode, divNode, ltNode, shiftNode, gtNode, joinNode, sliceNode, bitorNode,
  pairNode, andNode, orNode, ifNode, doNode, letNode, defNode, lambdaNode,
  callNode, refNode, errorNode, moduleNode, importNode, assignNode
} NodeType;

typedef struct ASTNode {
  NodeType type;
  u32 start;
  u32 end;
  union {
    u32 value;
    struct ASTNode **children;
  } data;
} ASTNode;
#define IsErrorNode(n)  ((n)->type == errorNode)

ASTNode *NewNode(NodeType type, u32 start, u32 end, u32 value);
ASTNode *CloneNode(ASTNode *node);
void FreeNode(ASTNode *node);
bool IsTerminal(ASTNode *node);
void NodePush(ASTNode *node, ASTNode *child);
void PrintNode(ASTNode *node);
