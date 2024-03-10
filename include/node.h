#pragma once

typedef enum {
  nilNode,
  idNode,
  varNode,
  intNode,
  floatNode,
  stringNode,
  symbolNode,
  listNode,
  tupleNode,
  pairNode,
  accessNode,
  negNode,
  addNode,
  subNode,
  mulNode,
  divNode,
  remNode,
  ltNode,
  ltEqNode,
  inNode,
  lenNode,
  eqNode,
  notEqNode,
  gtNode,
  gtEqNode,
  notNode,
  andNode,
  orNode,
  ifNode,
  doNode,
  callNode,
  lambdaNode,
  defNode,
  letNode,
  importNode,
  moduleNode
} NodeType;

/* Terminal nodes have values; non-terminals have children */
typedef struct {
  NodeType type;
  u32 pos;
} Node;

typedef struct {
  NodeType type;
  u32 pos;
  u32 value;
} TerminalNode;

typedef struct {
  NodeType type;
  u32 pos;
  Node **items;
} ListNode;

typedef struct {
  NodeType type;
  u32 pos;
  Node *child;
} UnaryNode;

typedef struct {
  NodeType type;
  u32 pos;
  Node *left;
  Node *right;
} BinaryNode;

typedef struct {
  NodeType type;
  u32 pos;
  Node *predicate;
  Node *ifTrue;
  Node *ifFalse;
} IfNode;

typedef struct {
  NodeType type;
  u32 pos;
  u32 *locals;
  Node **stmts;
} DoNode;

typedef struct {
  NodeType type;
  u32 pos;
  Node *op;
  Node **args;
} CallNode;

typedef struct {
  NodeType type;
  u32 pos;
  u32 *params;
  Node *body;
} LambdaNode;

typedef struct {
  NodeType type;
  u32 pos;
  u32 var;
  Node *value;
} LetNode;

typedef struct {
  NodeType type;
  u32 pos;
  u32 mod;
  u32 alias;
} ImportNode;

typedef struct {
  NodeType type;
  u32 pos;
  u32 name;
  u32 filename;
  ImportNode **imports;
  DoNode *body;
} ModuleNode;

TerminalNode *MakeTerminal(NodeType type, u32 pos, u32 value);
Node *MakeNode(NodeType type, u32 pos);
void FreeNode(Node *node);
bool IsTerminal(Node *node);
char *NodeName(Node *node);
void PrintAST(Node *ast);
