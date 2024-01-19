#pragma once
#include "lex.h"
#include "mem/mem.h"
#include "mem/symbols.h"
#include "univ/result.h"

#define SymBangEq       0x7FDD5C4E
#define SymHash         0x7FDF82DB
#define SymPercent      0x7FD3E679
#define SymAmpersand    0x7FDB283C
#define SymStar         0x7FD9A24B
#define SymPlus         0x7FD26AB0
#define SymMinus        0x7FD9FBF9
#define SymDot          0x7FD21A5F
#define SymDotDot       0x7FD62EE1
#define SymSlash        0x7FDDA21C
#define SymLt           0x7FDD1F00
#define SymLtLt         0x7FD72101
#define SymLtEq         0x7FDE01F2
#define SymLtGt         0x7FD3C54B
#define SymEqEq         0x7FDC5014
#define SymGt           0x7FD9FB4A
#define SymGtEq         0x7FD7C966
#define SymGtGt         0x7FDA0DDF
#define SymCaret        0x7FDC17FE
#define SymIn           0x7FD98998
#define SymNot          0x7FDBCA20
#define SymBar          0x7FDA1ADB
#define SymTilde        0x7FD373CF
#define SymSet          0x7FDB0EE9
#define MainModule      0x7FD09D97

typedef enum {
  NilNode,
  NumNode,
  SymbolNode,
  StringNode,
  IDNode,
  ListNode,
  TupleNode,
  MapNode,
  AndNode,
  OrNode,
  IfNode,
  DoNode,
  CallNode,
  LambdaNode,
  DefNode,
  LetNode,
  ExportNode,
  ImportNode,
  ModuleNode
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
#define ModuleFilename(mod)     NodeChild(mod, 3)
#define ModuleFile(mod)         NodeValue(ModuleFilename(mod))
#define ModuleSize(mod)         NodePos((Node*)NodeChild(mod, 3))
