#pragma once
#include "mem.h"

enum {nilNode, errNode, intNode, idNode, symNode, strNode, pairNode, joinNode,
  sliceNode, listNode, tupleNode, structNode, doNode, ifNode, andNode, orNode,
  eqNode, ltNode, gtNode, shiftNode, addNode, subNode, borNode, mulNode,
  divNode, remNode, bandNode, negNode, notNode, lenNode, compNode, callNode,
  accessNode, lambdaNode, letNode, defNode, importNode, exportNode, moduleNode};

val Parse(char *text);
void PrintAST(val node);
void PrintError(val node, char *source);

val MakeNode(i32 type, i32 start, i32 end, val value);
#define MakeError(msg, node) MakeNode(errNode, NodeStart(node), NodeEnd(node), SymVal(Symbol(msg)))

enum {start, end, type};

#define NodeType(node)      RawVal(TupleGet(node, 0))
#define NodeValue(node)     TupleGet(node, 1)
#define NodeProp(node,p)    TupleGet(node, (p)+2)
#define NodeStart(node)     RawVal(NodeProp(node, start))
#define NodeEnd(node)       RawVal(NodeProp(node, end))
#define NodeCount(node)     ListLength(NodeValue(node))
#define NodeChild(node, i)  ListGet(NodeValue(node), i)
#define IsError(node)       (IsTuple(node) && NodeType(node) == errNode)
#define ErrorMsg(node)      SymbolName(RawVal(NodeValue(node)))
char *NodeName(i32 type);
