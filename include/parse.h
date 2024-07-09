#pragma once
#include "mem.h"

enum {nilNode, errNode, intNode, idNode, symNode, strNode, pairNode, joinNode,
  sliceNode, listNode, tupleNode, structNode, doNode, ifNode, andNode, orNode,
  eqNode, ltNode, gtNode, shiftNode, addNode, subNode, borNode, mulNode,
  divNode, remNode, bandNode, negNode, notNode, lenNode, compNode, callNode,
  accessNode, lambdaNode, letNode, defNode, importNode, exportNode, moduleNode};

val Parse(char *text);
void PrintNode(val node, i32 level, u32 lines);
void PrintAST(val node);
void PrintError(char *prefix, val node, char *source);

val MakeNode(i32 type, i32 start, i32 end, val value);
#define MakeError(msg, start, end) MakeNode(errNode, start, end, SymVal(Symbol(msg)))

enum {startProp, endProp, typeProp};

#define NodeType(node)      RawVal(TupleGet(node, 0))
#define NodeValue(node)     TupleGet(node, 1)
#define NodeProp(node,p)    TupleGet(node, (p)+2)
#define SetNodeProp(n,p,v)  TupleSet(n, (p)+2, v)
#define NodeStart(node)     RawVal(NodeProp(node, startProp))
#define NodeEnd(node)       RawVal(NodeProp(node, endProp))
#define NodeCount(node)     ListLength(NodeValue(node))
#define NodeChild(node, i)  ListGet(NodeValue(node), i)
#define IsError(node)       (IsTuple(node) && NodeType(node) == errNode)
#define ErrorMsg(node)      SymbolName(RawVal(NodeValue(node)))
char *NodeName(i32 type);

#define IsModNode(node)     (NodeType(node) == moduleNode)
#define ModNodeName(node)   NodeValue(NodeChild(node, 0))
#define ModNodeExports(n)   NodeChild(node, 1)
#define ModNodeImports(n)   NodeChild(node, 2)
#define ModNodeBody(node)   NodeChild(node, 3)
