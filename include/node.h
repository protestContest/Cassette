#pragma once

/* An ASTNode is a node parsed from text. */

typedef enum {
  constNode, varNode, strNode, tupleNode, negNode, notNode, headNode, tailNode,
  lenNode, accessNode, compNode, eqNode, remNode, bitandNode, mulNode, addNode,
  subNode, divNode, ltNode, shiftNode, gtNode, joinNode, sliceNode, bitorNode,
  pairNode, andNode, orNode, ifNode, doNode, letNode, defNode, lambdaNode,
  paramsNode, callNode, refNode
} NodeType;

typedef struct ASTNode {
  NodeType type;
  char *file;
  u32 start;
  u32 end;
  union {
    u32 value;
    struct ASTNode **children;
  } data;
} ASTNode;

ASTNode *MakeNode(NodeType type, char *file, u32 start, u32 end);
ASTNode *MakeTerminal(NodeType type, char *file, u32 start, u32 end, u32 value);
ASTNode *CloneNode(ASTNode *node);
void FreeNode(ASTNode *node);
bool IsTerminal(ASTNode *node);
void NodePush(ASTNode *child, ASTNode *node);
void PrintNode(ASTNode *node);
