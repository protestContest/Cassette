#include "compile/node.h"
#include "runtime/symbol.h"

ASTNode *NewNode(NodeType type, u32 start, u32 end, u32 value)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->start = start;
  node->end = end;
  node->data.children = 0;
  node->data.value = value;
  node->attrs = 0;
  return node;
}

ASTNode *CloneNode(ASTNode *node)
{
  ASTNode *clone = NewNode(node->type, node->start, node->end, 0);
  if (IsTerminal(node)) {
    clone->data.value = node->data.value;
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
  switch (node->type) {
  case constNode:
  case idNode:
  case symNode:
  case strNode:
  case errorNode:
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

ASTNode *SimplifyNode(ASTNode *node, Env *env)
{
  switch (node->type) {
  case idNode: {
    u32 name = NodeValue(node);
    i32 pos = EnvFind(name, env);
    if (pos >= 0) {
      u32 value = EnvGet(pos, env);
      if (value != EnvUndefined) {
        NodeValue(node) = value;
        node->type = constNode;
      }
    }
    return node;
  }
  case lambdaNode:
    NodeChild(node, 1) = SimplifyNode(NodeChild(node, 1), env);
    return node;
  case negNode: {
    ASTNode *arg = NodeChild(node, 0);
    arg = SimplifyNode(arg, env);
    if (arg->type == constNode) {
      NodeValue(node) = IntVal(-RawInt(NodeValue(arg)));
      node->type = constNode;
      FreeNode(arg);
    } else {
      NodeChild(node, 0) = arg;
    }
    return node;
  }
  case notNode: {
    ASTNode *arg = NodeChild(node, 0);
    arg = SimplifyNode(arg, env);
    if (arg->type == constNode) {
      NodeValue(node) = IntVal(!RawInt(NodeValue(arg)));
      node->type = constNode;
      FreeNode(arg);
    } else {
      NodeChild(node, 0) = arg;
    }
    return node;
  }
  case compNode: {
    ASTNode *arg = NodeChild(node, 0);
    arg = SimplifyNode(arg, env);
    if (arg->type == constNode) {
      NodeValue(node) = IntVal(~RawInt(NodeValue(arg)));
      node->type = constNode;
      FreeNode(arg);
    } else {
      NodeChild(node, 0) = arg;
    }
    return node;
  }
  case eqNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) == RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case remNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) % RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case bitandNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) & RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case mulNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) * RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case addNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) + RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case subNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) - RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case divNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) / RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case ltNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) < RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case shiftNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) << RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case gtNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) > RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case bitorNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) | RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case xorNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode && arg2->type == constNode) {
      NodeValue(node) = IntVal(RawInt(NodeValue(arg1)) == RawInt(NodeValue(arg2)));
      node->type = constNode;
      FreeNode(arg1);
      FreeNode(arg2);
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
    }
    return node;
  }
  case andNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode) {
      FreeNode(node);
      if (NodeValue(arg1)) {
        FreeNode(arg1);
        return arg2;
      } else {
        FreeNode(node);
        FreeNode(arg2);
        return arg1;
      }
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
      return node;
    }
  }
  case orNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    if (arg1->type == constNode) {
      FreeNode(node);
      if (!NodeValue(arg1)) {
        FreeNode(arg1);
        return arg2;
      } else {
        FreeNode(node);
        FreeNode(arg2);
        return arg1;
      }
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
      return node;
    }
  }
  case refNode:
    NodeChild(node, 0) = SimplifyNode(NodeChild(node, 0), env);
    return node;
  case ifNode: {
    ASTNode *arg1 = NodeChild(node, 0);
    ASTNode *arg2 = NodeChild(node, 1);
    ASTNode *arg3 = NodeChild(node, 2);
    arg1 = SimplifyNode(arg1, env);
    arg2 = SimplifyNode(arg2, env);
    arg3 = SimplifyNode(arg3, env);
    if (arg1->type == constNode) {
      FreeNode(node);
      if (NodeValue(arg1)) {
        FreeNode(arg1);
        FreeNode(arg3);
        return arg2;
      } else {
        FreeNode(arg1);
        FreeNode(arg2);
        return arg3;
      }
    } else {
      NodeChild(node, 0) = arg1;
      NodeChild(node, 1) = arg2;
      NodeChild(node, 2) = arg3;
      return node;
    }
  }
  case letNode: {
    u32 num_assigns = GetNodeAttr(node, "count");
    env = ExtendEnv(num_assigns, env);
    NodeChild(node, 0) = SimplifyNode(NodeChild(node, 0), env);
    PopEnv(env);
    return node;
  }
  case assignNode: {
    u32 index;
    u32 var = NodeValue(NodeChild(node, 0));
    ASTNode *arg = SimplifyNode(NodeChild(node, 1), env);
    index = GetNodeAttr(node, "index");
    if (arg->type == constNode) {
      EnvSet(var, NodeValue(arg), index, env);
    } else {
      EnvSet(var, EnvUndefined, index, env);
    }
    NodeChild(node, 1) = arg;
    NodeChild(node, 2) = SimplifyNode(NodeChild(node, 2), env);
    return node;
  }
  case importNode:
    return node;
  case moduleNode:
    NodeChild(node, 3) = SimplifyNode(NodeChild(node, 3), env);
    return node;
  case errorNode:
    return node;
  default:
    if (!IsTerminal(node)) {
      u32 i;
      for (i = 0; i < NodeCount(node); i++) {
        NodeChild(node, i) = SimplifyNode(NodeChild(node, i), env);
      }
    }
    return node;
  }
}

#ifdef DEBUG
#include "runtime/mem.h"
#include "runtime/symbol.h"

static char *NodeTypeName(i32 type)
{
  switch (type) {
  case idNode:      return "id";
  case constNode:   return "const";
  case symNode:     return "sym";
  case strNode:     return "str";
  case tupleNode:   return "tuple";
  case lambdaNode:  return "lambda";
  case panicNode:   return "panic";
  case negNode:     return "neg";
  case notNode:     return "not";
  case headNode:    return "head";
  case tailNode:    return "tail";
  case lenNode:     return "len";
  case compNode:    return "comp";
  case eqNode:      return "eq";
  case remNode:     return "rem";
  case bitandNode:  return "bitand";
  case mulNode:     return "mul";
  case addNode:     return "add";
  case subNode:     return "sub";
  case divNode:     return "div";
  case ltNode:      return "lt";
  case shiftNode:   return "shift";
  case gtNode:      return "gt";
  case joinNode:    return "join";
  case sliceNode:   return "slice";
  case bitorNode:   return "bitor";
  case xorNode:     return "bitxor";
  case pairNode:    return "pair";
  case andNode:     return "and";
  case orNode:      return "or";
  case accessNode:  return "access";
  case callNode:    return "call";
  case refNode:     return "ref";
  case ifNode:      return "if";
  case letNode:     return "let";
  case assignNode:  return "assign";
  case doNode:      return "do";
  case defNode:     return "def";
  case importNode:  return "import";
  case moduleNode:  return "module";
  case errorNode:   return "error";
  default:          assert(false);
  }
}

void PrintNodeLevel(ASTNode *node, u32 level, u32 lines)
{
  u32 i, j;

  fprintf(stderr, "%s[%d:%d]",
    NodeTypeName(node->type), node->start, node->end);

  if (VecCount(node->attrs) > 0) {
    fprintf(stderr, " (");
    for (i = 0; i < VecCount(node->attrs); i++) {
      fprintf(stderr, "%s: %d",
        SymbolName(node->attrs[i].name), node->attrs[i].value);
      if (i < VecCount(node->attrs) - 1) fprintf(stderr, ", ");
    }
    fprintf(stderr, ")");
  }

  switch (node->type) {
  case constNode:
    if (node->data.value == 0) {
      fprintf(stderr, " nil\n");
    } else if (IsInt(node->data.value)) {
      fprintf(stderr, " %d\n", RawInt(node->data.value));
    } else {
      assert(false);
    }
    break;
  case idNode:
    fprintf(stderr, " %s\n", SymbolName(node->data.value));
    break;
  case symNode:
    fprintf(stderr, " %s\n", SymbolName(RawVal(node->data.value)));
    break;
  case strNode:
    fprintf(stderr, " \"%s\"\n", SymbolName(RawVal(node->data.value)));
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
