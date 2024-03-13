#pragma once

typedef enum {
  nilNode,
  idNode,
  intNode,
  floatNode,
  stringNode,
  listNode,
  tupleNode,
  doNode,
  ifNode,
  accessNode,
  callNode,
  negNode,
  notNode,
  bitNotNode,
  lenNode,
  mulNode,
  divNode,
  remNode,
  bitAndNode,
  addNode,
  subNode,
  bitOrNode,
  shiftNode,
  ltNode,
  gtNode,
  splitNode,
  tailNode,
  joinNode,
  pairNode,
  eqNode,
  andNode,
  orNode,
  lambdaNode,
  defNode,
  letNode,
  importNode,
  moduleNode
} NodeType;

typedef struct Node {
  NodeType type;
  u32 pos;
  struct Node **children;
  u32 value;
} Node;

#define NodePush(node, child)     VecPush((node)->children, child)
#define NodeCount(node)           VecCount((node)->children)
#define DoNodeLocals(node)        (node)->children[0]
#define DoNodeStmts(node)         (node)->children[1]
#define IfNodePredicate(node)     (node)->children[0]
#define IfNodeConsequent(node)    (node)->children[1]
#define IfNodeAlternative(node)   (node)->children[2]
#define LambdaNodeParams(node)    (node)->children[0]
#define LambdaNodeBody(node)      (node)->children[1]
#define AssignNodeVar(node)       (node)->children[0]
#define AssignNodeValue(node)     (node)->children[1]
#define ImportNodeMod(node)       (node)->children[0]
#define ImportNodeAlias(node)     (node)->children[1]
#define ModuleNodeName(node)      (node)->children[0]
#define ModuleNodeImports(node)   (node)->children[1]
#define ModuleNodeBody(node)      (node)->children[2]
#define ModuleNodeFilename(node)  (node)->children[3]

Node *MakeTerminal(NodeType type, u32 pos, u32 value);
Node *MakeNode(NodeType type, u32 pos);
void FreeNode(Node *node);
bool IsTerminal(Node *node);
char *NodeName(Node *node);
void PrintAST(Node *ast);
