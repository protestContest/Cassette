#pragma once
#include "mem.h"

enum {
  nilNode,
  errNode,
  intNode,
  idNode,
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

val Parse(char *text, i32 length);
void PrintAST(val node);
void PrintError(val node, char *source);

#define NodeType(node)      RawVal(ObjGet(node, 0))
#define NodePos(node)       RawVal(ObjGet(node, 1))
#define NodeValue(node)     ObjGet(node, 2)
#define NodeCount(node)     ListLength(NodeValue(node))
#define NodeChild(node, i)  ListGet(NodeValue(node), i)
#define IsError(node)       (NodeType(node) == errNode)
#define ErrorMsg(node)      SymbolName(RawVal(NodeValue(node)))
