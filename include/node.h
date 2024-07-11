#pragma once
#include "mem.h"

enum {nilNode, errNode, intNode, idNode, symNode, strNode, doNode, ifNode,
    listNode, andNode, orNode, callNode, lambdaNode, letNode, defNode,
    importNode, moduleNode};

val MakeNode(i32 type, i32 start, i32 end, val value);
#define MakeError(msg, start, end) \
  MakeNode(errNode, start, end, SymVal(Symbol(msg)))

void PrintNodeLevel(val node, i32 level, u32 lines);
void PrintNode(val node);
void PrintError(char *prefix, val node, char *source);

/*
Nodes are tuples with these fields:
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

#define IsError(node)       (IsTuple(node) && NodeType(node) == errNode)
#define ErrorMsg(node)      NodeText(node)

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

#define IsModNode(node)     (NodeType(node) == moduleNode)
#define ModNodeName(node)   NodeValue(NodeChild(node, 0))
#define ModNodeExports(n)   NodeChild(node, 1)
#define ModNodeImports(n)   NodeChild(node, 2)
#define ModNodeBody(node)   NodeChild(node, 3)
