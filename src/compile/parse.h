#pragma once
#include "mem/mem.h"
#include "lex.h"
#include "mem/symbols.h"
#include "result.h"

#if 0
#define SymEOF            0x7FDDD457 /* *eof* */
#define SymID             0x7FD7FE1D /* *id* */
#define SymBangEqual      0x7FDD5C4E /* != */
#define SymString         0x7FD9C953 /* " */
#define SymNewline        0x7FDB31E2 /* *nl* */
#define SymHash           0x7FDF82DB /* # */
#define SymPercent        0x7FD3E679 /* % */
#define SymAmpersand      0x7FDB283C /* & */
#define SymLParen         0x7FD2C1C9 /* ( */
#define SymRParen         0x7FDA8B60 /* ) */
#define SymStar           0x7FD9A24B /* * */
#define SymPlus           0x7FD26AB0 /* + */
#define SymComma          0x7FD86673 /* , */
#define SymMinus          0x7FD9FBF9 /* - */
#define SymArrow          0x7FDA0CB9 /* -> */
#define SymDot            0x7FD21A5F /* . */
#define SymSlash          0x7FDDA21C /* / */
#define SymNum            0x7FD6FB37 /* *num* */
#define SymColon          0x7FD7ED8B /* : */
#define SymLess           0x7FDD1F00 /* < */
#define SymLessLess       0x7FD72101 /* << */
#define SymLessEqual      0x7FDE01F2 /* <= */
#define SymLessGreater    0x7FD3C54B /* <> */
#define SymEqual          0x7FD43FF3 /* = */
#define SymEqualEqual     0x7FDC5014 /* == */
#define SymGreater        0x7FD9FB4A /* > */
#define SymGreaterEqual   0x7FD7C966 /* >= */
#define SymGreaterGreater 0x7FDA0DDF /* >> */
#define SymLBracket       0x7FDCA8DA /* [ */
#define SymBackslash      0x7FDAE079 /* \ */
#define SymRBracket       0x7FD4F739 /* ] */
#define SymCaret          0x7FDC17FE /* ^ */
#define SymAnd            0x7FDF9F13 /* and */
#define SymAs             0x7FD68A20 /* as */
#define SymCond           0x7FD1818B /* cond */
#define SymDef            0x7FD90914 /* def */
#define SymDo             0x7FDEFCF4 /* do */
#define SymElse           0x7FD0B8F8 /* else */
#define SymEnd            0x7FD0273A /* end */
#define SymFalse          0x7FDAF4C4 /* false */
#define SymIf             0x7FDE93D3 /* if */
#define SymImport         0x7FD8B92B /* import */
#define SymIn             0x7FD98998 /* in */
#define SymLet            0x7FDAE6A4 /* let */
#define SymModule         0x7FD2502A /* module */
#define SymNil            0x7FD5AFC3 /* nil */
#define SymNot            0x7FDBCA20 /* not */
#define SymOr             0x7FD143B9 /* or */
#define SymTrue           0x7FD7E33F /* true */
#define SymLBrace         0x7FD99DE3 /* { */
#define SymBar            0x7FDA1ADB /* | */
#define SymRBrace         0x7FD3CE87 /* } */
#define SymTilde          0x7FD373CF /* ~ */
#endif

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
  NotEqNode,
  RemNode,
  BitAndNode,
  MultiplyNode,
  AddNode,
  SubtractNode,
  DivideNode,
  LtNode,
  LShiftNode,
  LtEqNode,
  CatNode,
  EqNode,
  GtNode,
  GtEqNode,
  RShiftNode,
  BitOrNode,
  AndNode,
  InNode,
  OrNode,
  PairNode,
  LengthNode,
  NegativeNode,
  NotNode,
  BitNotNode
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

void InitParser(Parser *p, SymbolTable *symbols);
Result Parse(char *source, Parser *p);


Node *MakeTerminal(NodeType type, u32 position, Val value);
Node *MakeNode(NodeType type, u32 position);
void SetNodeType(Node *node, NodeType type);
void FreeNode(Node *node);
void FreeAST(Node *node);
bool IsTerminal(Node *node);

#define NodePush(node, child)   ObjVecPush(&(node)->expr.children, child)
#define NodeChild(node, i)      (((Node*)(node))->expr.children.items[i])
#define NumNodeChildren(node)   ((node)->expr.children.count)
#define NodeValue(node)         (((Node*)(node))->expr.value)

#define ModuleName(mod)         NodeValue(NodeChild(mod, 0))
#define ModuleImports(mod)      NodeChild(mod, 1)
#define ModuleBody(mod)         NodeChild(mod, 2)
#define ModuleExports(mod)      NodeChild(ModuleBody(mod), 0)
#define ModuleFile(mod)         NodeValue(NodeChild(mod, 3))
