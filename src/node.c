#include "node.h"
#include "univ/symbol.h"

ASTNode *NewNode(NodeType type, u32 start, u32 end)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->start = start;
  node->end = end;
  InitVec(node->data.children);
  InitVec(node->attrs);
  return node;
}

ASTNode *NewTerminal(NodeType type, u32 start, u32 end, u32 value)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->start = start;
  node->end = end;
  node->data.value = value;
  InitVec(node->attrs);
  return node;
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

ASTNode *CloneNode(ASTNode *node)
{
  ASTNode *clone;
  if (IsTerminal(node)) {
    clone = NewTerminal(node->type, node->start, node->end, node->data.value);
  } else {
    u32 i;
    clone = NewNode(node->type, node->start, node->end);
    for (i = 0; i < VecCount(node->data.children); i++) {
      NodePush(clone, CloneNode(VecAt(node->data.children, i)));
    }
  }
  return clone;
}

void FreeNode(ASTNode *node)
{
  u32 i;
  if (!IsTerminal(node)) {
    for (i = 0; i < VecCount(node->data.children); i++) {
      FreeNode(VecAt(node->data.children, i));
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

void NodePush(ASTNode *node, ASTNode *child)
{
  VecPush(node->data.children, child);
  if (child->end > node->end) node->end = child->end;
}

char *NodeTypeName(i32 type)
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
  case trapNode:    return "trap";
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

void SetNodeAttr(ASTNode *node, char *name, u32 value)
{
  u32 i;
  u32 key = Symbol(name);
  NodeAttr attr;
  for (i = 0; i < VecCount(node->attrs); i++) {
    if (VecAt(node->attrs, i).name == key) {
      VecAt(node->attrs, i).value = value;
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
    if (VecAt(node->attrs, i).name == key) {
      return VecAt(node->attrs, i).value;
    }
  }
  assert(false);
}

#ifdef DEBUG
#include "mem.h"
#include "univ/symbol.h"

void PrintNodeLevel(ASTNode *node, u32 level, u32 lines)
{
  u32 i, j;

  fprintf(stderr, "%s[%d:%d]", NodeTypeName(node->type), node->start, node->end);

  if (VecCount(node->attrs) > 0) {
    fprintf(stderr, " (");
    for (i = 0; i < VecCount(node->attrs); i++) {
      fprintf(stderr, "%s: %d",
          SymbolName(VecAt(node->attrs, i).name),
          VecAt(node->attrs, i).value);
      if (i < VecCount(node->attrs) - 1) fprintf(stderr, ", ");
    }
    fprintf(stderr, ")");
  }

  switch (node->type) {
  case constNode:
    if (node->data.value == 0) {
      fprintf(stderr, " nil\n");
    } else if (IsInt(node->data.value)) {
      char *name = SymbolName(RawVal(node->data.value));
      if (name) {
        fprintf(stderr, " :%s\n", name);
      } else {
        fprintf(stderr, " %d\n", RawInt(node->data.value));
      }
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
      PrintNodeLevel(VecAt(node->data.children, i), level+1, lines);
    }
  }
}

void PrintNode(ASTNode *node)
{
  PrintNodeLevel(node, 0, 0);
}
#endif
