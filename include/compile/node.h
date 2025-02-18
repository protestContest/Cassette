#pragma once
#include "compile/env.h"
#include "univ/vec.h"

/*
An ASTNode is a node parsed from text.

Node structures:

errorNode   Terminal, message (symbol)
nilNode     Terminal, nil
intNode     Terminal, integer
symNode     Terminal, symbol
strNode     Terminal, symbol
idNode      Terminal, symbol

tupleNode   List of items
recordNode  0: name (idNode)
            1: field names (listNode of idNode)
listNode    List of items
lambdaNode  0: params (listNode of idNode)
            1: body node

opNode:     List of args
callNode:   0: function
            1: args (listNode)
refNode:    0: arg1
            1: arg2
andNode:    0: arg1
            1: arg2
orNode:     0: arg1
            1: arg2
ifNode      0: predicate
            1: consequent
            2: alternative
assignNode  0: var (idNode)
            1: value
letNode     0: assigns (listNode of assignNode)
            1: expr
defNode     0: name (idNode)
            1: value (lambdaNode)
doNode      List of stmts
importNode  0: name (idNode)
            1: alias (idNode)
            2: functions (listNode of idNode)
moduleNode  0: name (idNode)
            1: imports (listNode of importNode)
            2: exports (tupleNode of idNode)
            3: body (doNode)
*/

typedef enum {
  errorNode,
  nilNode,
  intNode,
  symNode,
  strNode,
  idNode,
  tupleNode,
  recordNode,
  listNode,
  lambdaNode,
  opNode,
  callNode,
  refNode,
  andNode,
  orNode,
  ifNode,
  assignNode,
  letNode,
  doNode,
  importNode,
  moduleNode
} NodeType;

typedef struct {
  u32 name;
  u32 value;
} NodeAttr;


typedef struct ASTNode {
  NodeType nodeType;
  u32 start;
  u32 end;
  union {
    u32 value;
    struct ASTNode **children; /* vec */
  } data;
  NodeAttr *attrs; /* vec */
} ASTNode;
#define IsErrorNode(n)  ((n)->nodeType == errorNode)
#define NodeCount(n)    VecCount((n)->data.children)
#define NodeChild(n,i)  ((n)->data.children[i])
#define NodeValue(n)    ((n)->data.value)
#define IsNodeFalse(n)   \
  ((n)->nodeType == nilNode || ((n)->nodeType == intNode && RawInt(NodeValue(n)) == 0))

ASTNode *NewNode(NodeType type, u32 start, u32 end, u32 value);
#define ErrorNode(msg, start, end) NewNode(errorNode, start, end, Symbol(msg))
ASTNode *CloneNode(ASTNode *node);
void FreeNode(ASTNode *node);
void FreeNodeShallow(ASTNode *node);
bool IsTerminal(ASTNode *node);
bool IsConstNode(ASTNode *node);
void NodePush(ASTNode *node, ASTNode *child);
void SetNodeAttr(ASTNode *node, char *name, u32 value);
u32 GetNodeAttr(ASTNode *node, char *name);

ASTNode *SimplifyNode(ASTNode *node, Env *env);

#ifdef DEBUG
void PrintNode(ASTNode *node);
#endif
