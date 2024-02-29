#pragma once
#include "univ/vec.h"

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
typedef struct {
  NodeType type;
  u32 pos;
  union {
    i64 intval;
    double floatval;
    void *value;
    ObjVec children;
  } expr;
} Node;

Node *MakeTerminal(NodeType type, u32 position, void *value);
Node *MakeNode(NodeType type, u32 position);
Node *MakeIntNode(u32 position, i64 value);
Node *MakeFloatNode(u32 position, double value);
void FreeNode(Node *node);
void FreeAST(Node *node);
void SetNodeType(Node *node, NodeType type);
bool IsTerminal(Node *node);
void PrintAST(Node *ast);

#define NodePush(node, child)   ObjVecPush(&(node)->expr.children, child)
#define NodeChild(node, i)      (((Node*)(node))->expr.children.items[i])
#define NumNodeChildren(node)   ((node)->expr.children.count)
#define NodeValue(node)         (((Node*)(node))->expr.value)
#define NodePos(node)           ((node)->pos)

#define ModuleName(mod)         NodeValue(NodeChild(mod, 0))
#define ModuleImports(mod)      NodeChild(mod, 1)
#define ModuleBody(mod)         NodeChild(mod, 2)
#define ModuleExports(mod)      NodeChild(ModuleBody(mod), 0)
#define ModuleFilename(mod)     NodeChild(mod, 3)
#define ModuleFile(mod)         NodeValue(ModuleFilename(mod))
#define ModuleSize(mod)         NodePos((Node*)NodeChild(mod, 3))
