#pragma once

typedef enum {
  /* Terminal nodes */
  NilNode,
  IDNode,
  IntNode,
  FloatNode,
  StringNode,
  SymbolNode,

  /* Non-terminal nodes */
  ListNode,
  TupleNode,
  MapNode,
  PairNode,
  AccessNode,
  NegNode,
  AddNode,
  SubNode,
  MulNode,
  DivNode,
  RemNode,
  LtNode,
  LtEqNode,
  InNode,
  LenNode,
  EqNode,
  NotEqNode,
  GtNode,
  GtEqNode,
  NotNode,
  AndNode,
  OrNode,
  IfNode,
  DoNode,
  CallNode,
  LambdaNode,
  DefNode,
  LetNode,
  SetNode,
  ExportNode,
  ImportNode,
  ModuleNode
} NodeType;

/* Terminal nodes have values; non-terminals have children */
typedef struct Node {
  NodeType type;
  u32 pos;
  union {
    u32 value;
    struct Node **children;
  } expr;
} Node;

Node *MakeTerminal(NodeType type, u32 position, u32 value);
Node *MakeNode(NodeType type, u32 position);
Node *MakeIntNode(u32 position, i64 value);
Node *MakeFloatNode(u32 position, double value);
void FreeNode(Node *node);
void FreeAST(Node *node);
void SetNodeType(Node *node, NodeType type);
bool IsTerminal(Node *node);
char *NodeName(Node *node);
void PrintAST(Node *ast);

#define NodePush(node, child)   VecPush((node)->expr.children, child)
#define NodeChild(node, i)      (((Node*)(node))->expr.children[i])
#define NodeCount(node)         VecCount((node)->expr.children)
#define NodeValue(node)         (((Node*)(node))->expr.value)
#define NodePos(node)           ((node)->pos)

#define ModuleName(mod)         NodeValue(NodeChild(mod, 0))
#define ModuleImports(mod)      NodeChild(mod, 1)
#define ModuleBody(mod)         NodeChild(mod, 2)
#define ModuleExports(mod)      NodeChild(ModuleBody(mod), 0)
#define ModuleFilename(mod)     NodeChild(mod, 3)
#define ModuleFile(mod)         NodeValue(ModuleFilename(mod))
#define ModuleSize(mod)         NodePos((Node*)NodeChild(mod, 3))
