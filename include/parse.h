#pragma once
#include "mem.h"

enum {
  errorNode,
  nilNode,
  idNode,
  numNode,
  symNode,
  strNode,
  listNode,
  tupleNode,
  doNode,
  ifNode,
  andNode,
  orNode,
  eqNode,
  ltNode,
  gtNode,
  addNode,
  subNode,
  mulNode,
  divNode,
  remNode,
  negNode,
  notNode,
  callNode,
  accessNode,
  lambdaNode,
  letNode,
  defNode,
  importNode,
  exportNode,
  moduleNode
};

i32 Parse(char *text, i32 length);
void PrintAST(i32 node);
void PrintError(i32 node, char *source);

#define NodeType(node)      RawVal(ObjGet(node, 0))
#define NodePos(node)       RawVal(ObjGet(node, 1))
#define NodeValue(node)     ObjGet(node, 2)
#define NodeCount(node)     ListLength(NodeValue(node))
#define NodeChild(node, i)  ListGet(NodeValue(node), i)
#define IsError(node)       (NodeType(node) == errorNode)
#define ErrorMsg(node)      SymbolName(RawVal(NodeValue(node)))
