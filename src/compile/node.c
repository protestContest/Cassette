#include "compile/node.h"
#include "runtime/ops.h"
#include "runtime/symbol.h"

ASTNode *NewNode(NodeType type, u32 start, u32 end, u32 value)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->nodeType = type;
  node->start = start;
  node->end = end;
  node->data.children = 0;
  node->data.value = value;
  node->attrs = 0;
  return node;
}

ASTNode *CloneNode(ASTNode *node)
{
  ASTNode *clone = NewNode(node->nodeType, node->start, node->end, 0);
  if (IsTerminal(node)) {
    NodeValue(clone) = NodeValue(node);
  } else {
    u32 i;
    for (i = 0; i < VecCount(node->data.children); i++) {
      NodePush(clone, CloneNode(node->data.children[i]));
    }
  }
  return clone;
}

void FreeNode(ASTNode *node)
{
  u32 i;
  if (!IsTerminal(node)) {
    for (i = 0; i < VecCount(node->data.children); i++) {
      FreeNode(node->data.children[i]);
    }
    FreeVec(node->data.children);
  }
  FreeVec(node->attrs);
  free(node);
}

void FreeNodeShallow(ASTNode *node)
{
  if (!IsTerminal(node)) {
    FreeVec(node->data.children);
  }
  FreeVec(node->attrs);
  free(node);
}

bool IsTerminal(ASTNode *node)
{
  switch (node->nodeType) {
  case errorNode:
  case nilNode:
  case intNode:
  case symNode:
  case strNode:
  case idNode:
    return true;
  default:
    return false;
  }
}

bool IsConstNode(ASTNode *node)
{
  switch (node->nodeType) {
  case nilNode:
  case intNode:
  case symNode:
  case strNode:
    return true;
  default:
    return false;
  }
}

void NodePush(ASTNode *node, ASTNode *child)
{
  VecPush(node->data.children, child);
  if (child->end > node->end) node->end = child->end;
}

void SetNodeAttr(ASTNode *node, char *name, u32 value)
{
  u32 i;
  u32 key = Symbol(name);
  NodeAttr attr;
  for (i = 0; i < VecCount(node->attrs); i++) {
    if (node->attrs[i].name == key) {
      node->attrs[i].value = value;
      return;
    }
  }
  attr.name = key;
  attr.value = value;
  VecPush(node->attrs, attr);
}

u32 GetNodeAttr(ASTNode *node, char *name)
{
  u32 i;
  u32 key = Symbol(name);
  for (i = 0; i < VecCount(node->attrs); i++) {
    if (node->attrs[i].name == key) {
      return node->attrs[i].value;
    }
  }
  assert(false);
}

/* Simplifies constant arithemtic and logic expressions, including when they're
 * saved as variables */
ASTNode *SimplifyNode(ASTNode *node, Env *env)
{
  u32 start = node->start, end = node->end;

  switch (node->nodeType) {
  case errorNode:
  case nilNode:
  case intNode:
  case symNode:
  case strNode:
  case recordNode:
  case refNode:
  case importNode:
    return node;

  case idNode: {
    u32 name = NodeValue(node);
    i32 pos = EnvFind(name, env);
    if (pos >= 0) {
      u32 value = EnvGet(pos, env);
      if (value != EnvUndefined) {
        NodeValue(node) = value;
        node->nodeType = value ? intNode : nilNode;
      }
    }
    return node;
  }

  case lambdaNode: {
    ASTNode *params = NodeChild(node, 0);
    u32 i;
    env = ExtendEnv(NodeCount(params), env);
    for (i = 0; i < NodeCount(params); i++) {
      EnvSet(NodeValue(NodeChild(params, i)), EnvUndefined, i, env);
    }
    NodeChild(node, 1) = SimplifyNode(NodeChild(node, 1), env);
    env = PopEnv(env);
    return node;
  }

  case opNode: {
    u32 op = GetNodeAttr(node, "opCode");
    ASTNode *arg1, *arg2;
    switch (op) {
    case opNeg:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      if (arg1->nodeType == intNode) {
        NodeValue(arg1) = IntVal(-RawInt(NodeValue(arg1)));
        FreeNodeShallow(node);
        node = arg1;
      }
      return node;
    case opNot:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      if (IsConstNode(arg1)) {
        if (IsNodeFalse(arg1)) {
          FreeNode(node);
          return NewNode(intNode, start, end, IntVal(1));
        } else {
          FreeNode(node);
          return NewNode(intNode, start, end, IntVal(0));
        }
      }
      return node;
    case opComp:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      if (arg1->nodeType == intNode) {
        u32 value = NodeValue(arg1);
        FreeNode(node);
        return NewNode(intNode, start, end, IntVal(~RawInt(value)));
      }
      return node;
    case opEq:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == nilNode && arg2->nodeType == nilNode) {
        FreeNode(node);
        return NewNode(intNode, start, end, IntVal(1));
      }
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        bool eq = NodeValue(arg1) == NodeValue(arg2);
        FreeNode(node);
        return NewNode(intNode, start, end, IntVal(eq));
      }
      return node;
    case opRem:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) % RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opAnd:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) & RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opMul:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) * RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opAdd:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) + RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opSub:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) - RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opDiv:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) / RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opLt:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) < RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opShift:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) << RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opGt:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) > RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opOr:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) | RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    case opXor:
      arg1 = SimplifyNode(NodeChild(node, 0), env);
      arg2 = SimplifyNode(NodeChild(node, 1), env);
      if (arg1->nodeType == intNode && arg2->nodeType == intNode) {
        u32 value = IntVal(RawInt(NodeValue(arg1)) ^ RawInt(NodeValue(arg2)));
        FreeNode(node);
        return NewNode(intNode, start, end, value);
      }
      return node;
    default:
      return node;
    }
  }

  case andNode: {
    ASTNode *arg1 = SimplifyNode(NodeChild(node, 0), env);
    ASTNode *arg2 = SimplifyNode(NodeChild(node, 1), env);
    if (IsConstNode(arg1)) {
      if (IsNodeFalse(arg1)) {
        FreeNodeShallow(node);
        FreeNode(arg2);
        return arg1;
      } else {
        FreeNodeShallow(node);
        FreeNode(arg1);
        return arg2;
      }
    }
    return node;
  }

  case orNode: {
    ASTNode *arg1 = SimplifyNode(NodeChild(node, 0), env);
    ASTNode *arg2 = SimplifyNode(NodeChild(node, 1), env);
    if (IsConstNode(arg1)) {
      if (IsNodeFalse(arg1)) {
        FreeNodeShallow(node);
        FreeNode(arg1);
        return arg2;
      } else {
        FreeNodeShallow(node);
        FreeNode(arg2);
        return arg1;
      }
    }
    return node;
  }

  case ifNode: {
    ASTNode *arg1, *arg2, *arg3;
    arg1 = SimplifyNode(NodeChild(node, 0), env);
    arg2 = SimplifyNode(NodeChild(node, 1), env);
    arg3 = SimplifyNode(NodeChild(node, 2), env);
    if (IsNodeFalse(arg1)) {
      FreeNodeShallow(node);
      FreeNode(arg1);
      FreeNode(arg2);
      return arg3;
    } else if (IsConstNode(arg1)) {
      FreeNodeShallow(node);
      FreeNode(arg1);
      FreeNode(arg3);
      return arg2;
    }
    NodeChild(node, 0) = arg1;
    NodeChild(node, 1) = arg2;
    NodeChild(node, 2) = arg3;
    return node;
  }

  case assignNode: {
    u32 index = GetNodeAttr(node, "index");
    u32 var = NodeValue(NodeChild(node, 0));
    ASTNode *value = SimplifyNode(NodeChild(node, 1), env);
    if (value->nodeType == intNode || value->nodeType == nilNode) {
      EnvSet(var, NodeValue(value), index, env);
    } else {
      EnvSet(var, EnvUndefined, index, env);
    }
    NodeChild(node, 1) = value;
    return node;
  }

  case letNode: {
    u32 numAssigns = NodeCount(NodeChild(node, 0));
    u32 i;
    env = ExtendEnv(numAssigns, env);
    for (i = 0; i < NodeCount(node); i++) {
      NodeChild(node, i) = SimplifyNode(NodeChild(node, i), env);
    }
    env = PopEnv(env);
    return node;
  }

  case doNode: {
    u32 numAssigns = GetNodeAttr(node, "numAssigns");
    u32 i;

    if (numAssigns == 0 && NodeCount(node) == 1) {
      ASTNode *stmt = SimplifyNode(NodeChild(node, 0), env);
      FreeNodeShallow(node);
      return stmt;
    }

    env = ExtendEnv(numAssigns, env);
    for (i = 0; i < numAssigns; i++) {
      ASTNode *child = NodeChild(node, i);
      u32 index, var;
      assert(child->nodeType == assignNode);
      index = GetNodeAttr(child, "index");
      var = NodeValue(NodeChild(child, 0));
      EnvSet(var, EnvUndefined, index, env);
    }
    for (i = 0; i < NodeCount(node); i++) {
      NodeChild(node, i) = SimplifyNode(NodeChild(node, i), env);
    }
    PopEnv(env);
    return node;
  }

  case moduleNode:
    NodeChild(node, 3) = SimplifyNode(NodeChild(node, 3), env);
    return node;

  default:
    {
      u32 i;
      for (i = 0; i < NodeCount(node); i++) {
        NodeChild(node, i) = SimplifyNode(NodeChild(node, i), env);
      }
      return node;
    }
  }
}

#ifdef DEBUG
#include "runtime/mem.h"
#include "runtime/symbol.h"

static char *NodeTypeName(i32 type)
{
  switch (type) {
  case errorNode:   return "error";
  case nilNode:     return "nil";
  case intNode:     return "int";
  case symNode:     return "sym";
  case strNode:     return "str";
  case idNode:      return "id";
  case tupleNode:   return "tuple";
  case recordNode:  return "record";
  case listNode:    return "list";
  case lambdaNode:  return "lambda";
  case opNode:      return "op";
  case callNode:    return "call";
  case refNode:     return "ref";
  case andNode:     return "and";
  case orNode:      return "or";
  case ifNode:      return "if";
  case assignNode:  return "assign";
  case letNode:     return "let";
  case doNode:      return "do";
  case importNode:  return "import";
  case moduleNode:  return "module";
  default:          assert(false);
  }
}

void PrintNodeLevel(ASTNode *node, u32 level, u32 lines)
{
  u32 i, j;

  fprintf(stderr, "%s[%d:%d]",
    NodeTypeName(node->nodeType), node->start, node->end);

  if (node->nodeType == opNode) {
    u32 opCode = GetNodeAttr(node, "opCode");
    fprintf(stderr, "<%s>", OpName(opCode));
  }

/*
  if (node->type) {
    fprintf(stderr, ": ");
    PrintType(node->type);
  }
*/

  if (VecCount(node->attrs) > 0) {
    fprintf(stderr, " (");
    for (i = 0; i < VecCount(node->attrs); i++) {
      char *value_str = SymbolName(node->attrs[i].value);
      if (value_str) {
        fprintf(stderr, "%s: %s",
          SymbolName(node->attrs[i].name), value_str);
      } else {
        fprintf(stderr, "%s: %d",
          SymbolName(node->attrs[i].name), node->attrs[i].value);
      }
      if (i < VecCount(node->attrs) - 1) fprintf(stderr, ", ");
    }
    fprintf(stderr, ")");
  }

  switch (node->nodeType) {
  case errorNode:
    fprintf(stderr, " \"%s\"\n", SymbolName(NodeValue(node)));
    break;
  case nilNode:
    fprintf(stderr, " nil\n");
    break;
  case intNode:
    fprintf(stderr, " %d\n", RawInt(NodeValue(node)));
    break;
  case symNode:
    fprintf(stderr, " %s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case strNode:
    fprintf(stderr, " \"%s\"\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case idNode:
    fprintf(stderr, " %s\n", SymbolName(NodeValue(node)));
    break;
  default:
    fprintf(stderr, "\n");
    lines |= 1 << level;
    for (i = 0; i < VecCount(node->data.children); i++) {
      for (j = 0; j < level; j++) {
        if (lines & (1 << j)) {
          fprintf(stderr, "│ ");
        } else {
          fprintf(stderr, "  ");
        }
      }
      if (i < VecCount(node->data.children) - 1) {
        fprintf(stderr, "├ ");
      } else {
        lines &= ~(1 << level);
        fprintf(stderr, "└ ");
      }
      PrintNodeLevel(node->data.children[i], level+1, lines);
    }
  }
}

void PrintNode(ASTNode *node)
{
  PrintNodeLevel(node, 0, 0);
}
#endif
