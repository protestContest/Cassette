#pragma once
#include "mem/mem.h"
#include "lex.h"
#include "mem/symbols.h"
#include "result.h"

typedef enum {
  ModuleNode,
  ImportNode,
  LetNode,
  DefNode,
  SymbolNode,
  DoNode,
  LambdaNode,
  CallNode,
  NilNode,
  IfNode,
  ListNode,
  MapNode,
  TupleNode,
  IDNode,
  NumNode,
  StringNode,
  AndNode,
  OrNode
} NodeType;

typedef struct {
  NodeType type;
  u32 pos;
  union {
    Val value;
    ObjVec children;
  } expr;
} Node;

typedef struct {
  char *filename;
  Lexer lex;
  SymbolTable *symbols;
} Parser;

Result ParseFile(char *filename, SymbolTable *symbols);
void InitParser(Parser *p, SymbolTable *symbols);
Result Parse(char *source, Parser *p);
void FreeAST(Node *node);

#define NodePush(node, child)   ObjVecPush(&(node)->expr.children, child)
#define NodeChild(node, i)      (((Node*)(node))->expr.children.items[i])
#define NumNodeChildren(node)   ((node)->expr.children.count)
#define NodeValue(node)         (((Node*)(node))->expr.value)
#define NodePos(node)           ((node)->pos)

#define ModuleName(mod)         NodeValue(NodeChild(mod, 0))
#define ModuleImports(mod)      NodeChild(mod, 1)
#define ModuleBody(mod)         NodeChild(mod, 2)
#define ModuleExports(mod)      NodeChild(ModuleBody(mod), 0)
#define ModuleFile(mod)         NodeValue(NodeChild(mod, 3))
#define ModuleSize(mod)         NodePos((Node*)NodeChild(mod, 3))
