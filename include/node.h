#pragma once

/*
A node is an element in an abstract syntax tree. Nodes are represented as tuples
with these fields:
- node type
- node value (varies by node type):
  - nilNode: nothing
  - errNode: symbol of error message
  - intNode: integer
  - idNode: symbol of ID
  - symNode: symbol
  - strNode: symbol of text
  - doNode: list of [# of assigns, ...stmts]
  - ifNode: list of [predicate, consequent, alternative]
  - andNode: list of operands
  - orNode: list of operands
  - callNode: list of [function, ...arguments]
  - lambdaNode: list of [[...params], body]
  - letNode: list of [var, expr]
  - defNode: list of [var, expr]
  - importNode: list of [mod name, alias]
  - moduleNode: list of [mod name, [...export IDs], [...imports], body]
- start pos
- end pos
- inferred type
*/

#include "mem.h"

enum {errNode, idNode, nilNode, intNode, symNode, strNode, listNode, tupleNode,
  doNode, ifNode, lambdaNode, notNode, lenNode, compNode, negNode, accessNode,
  sliceNode, mulNode, divNode, remNode, bitandNode, subNode, addNode, bitorNode,
  shiftNode, ltNode, gtNode, joinNode, pairNode, eqNode, andNode, orNode,
  callNode, letNode, defNode, importNode, moduleNode};

typedef val Node;

Node MakeNode(i32 type, i32 start, i32 end, val value);
void PrintNodeLevel(Node node, i32 level, u32 lines);
void PrintNode(Node node);

#define NodeType(node)      RawVal(TupleGet(node, 0))
#define NodeValue(node)     TupleGet(node, 1)
#define NodeStart(node)     RawVal(TupleGet(node, 2))
#define SetNodeStart(n,i)   TupleSet(n, 2, IntVal(i))
#define NodeEnd(node)       RawVal(TupleGet(node, 3))
#define SetNodeEnd(n, i)    TupleSet(n, 3, IntVal(i))
#define NodeValType(node)   TupleGet(node, 4)
#define SetNodeType(n, t)   TupleSet(n, 4, t)

#define NodeChildren(node)  NodeValue(node)
#define NodeLength(node)    ListLength(NodeChildren(node))
#define NodeChild(node, i)  ListGet(NodeChildren(node), i)

#define NodeText(node)      SymbolName(RawVal(NodeValue(node)))
#define NodeInt(node)       RawInt(NodeValue(node))

#define DoNodeNumAssigns(n) NodeInt(NodeChild(n, 0))
#define DoNodeStmts(node)   Tail(NodeChildren(node))
#define CallNodeFun(node)   NodeChild(node, 0)
#define CallNodeArgs(node)  Tail(NodeChildren(node))
#define LambdaNodeParams(n) NodeValue(NodeChild(n, 0))
#define LambdaNodeBody(n)   NodeChild(n, 1)
#define LetNodeVar(node)    NodeChild(node, 0)
#define LetNodeExpr(node)   NodeChild(node, 1)
#define DefNodeVar(node)    NodeChild(node, 0)
#define DefNodeExpr(node)   NodeChild(node, 1)
#define ImportNodeName(n)   NodeChild(n, 0)
#define ImportNodeAlias(n)  NodeChild(n, 1)
#define IfNodePred(node)    NodeChild(node, 0)
#define IfNodeCons(node)    NodeChild(node, 1)
#define IfNodeAlt(node)    NodeChild(node, 2)

#define IsModNode(node)     (NodeType(node) == moduleNode)
#define ModNodeName(node)   NodeValue(NodeChild(node, 0))
#define ModNodeExports(n)   NodeChild(node, 1)
#define ModNodeImports(n)   NodeChild(node, 2)
#define ModNodeBody(node)   NodeChild(node, 3)
