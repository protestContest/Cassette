#include "node.h"
#include "mem.h"
#include "univ/math.h"
#include "univ/symbol.h"
#include "univ/vec.h"

ASTNode *MakeTerminal(NodeType type, char *file, u32 start, u32 end, u32 value)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->file = file;
  node->start = start;
  node->end = end;
  node->data.value = value;
  return node;
}

ASTNode *MakeNode(NodeType type, char *file, u32 start, u32 end)
{
  ASTNode *node = malloc(sizeof(ASTNode));
  node->type = type;
  node->file = file;
  node->start = start;
  node->end = end;
  node->data.children = 0;
  return node;
}

bool IsTerminal(ASTNode *node)
{
  switch (node->type) {
  case constNode:
  case varNode:
  case strNode:
    return true;
  default:
    return false;
  }
}

ASTNode *CloneNode(ASTNode *node)
{
  ASTNode *clone = MakeNode(node->type, node->file, node->start, node->end);
  if (IsTerminal(node)) {
    clone->data.value = node->data.value;
  } else {
    u32 i;
    for (i = 0; i < VecCount(node->data.children); i++) {
      NodePush(CloneNode(node->data.children[i]), clone);
    }
  }
  return clone;
}

void FreeNode(ASTNode *node)
{
  if (!IsTerminal(node)) {
    u32 i;
    for (i = 0; i < VecCount(node->data.children); i++) {
      FreeNode(node->data.children[i]);
    }
    FreeVec(node->data.children);
  }
  free(node);
}

void NodePush(ASTNode *child, ASTNode *node)
{
  VecPush(node->data.children, child);
}

char *NodeTypeName(i32 type)
{
  switch (type) {
  case constNode:   return "const";
  case varNode:     return "var";
  case strNode:     return "str";
  case tupleNode:   return "tuple";
  case notNode:     return "not";
  case lenNode:     return "len";
  case compNode:    return "comp";
  case negNode:     return "neg";
  case accessNode:  return "access";
  case eqNode:      return "eq";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case bitandNode:  return "bitand";
  case subNode:     return "sub";
  case addNode:     return "add";
  case shiftNode:   return "shift";
  case ltNode:      return "lt";
  case gtNode:      return "gt";
  case sliceNode:   return "slice";
  case bitorNode:   return "bitor";
  case joinNode:    return "join";
  case pairNode:    return "pair";
  case andNode:     return "and";
  case orNode:      return "or";
  case ifNode:      return "if";
  case doNode:      return "do";
  case letNode:     return "let";
  case defNode:     return "def";
  case lambdaNode:  return "lambda";
  case paramsNode:  return "params";
  case callNode:    return "call";
  case refNode:     return "ref";
  default: assert(false);
  }
}

void PrintNodeLevel(ASTNode *node, u32 level, u32 lines)
{
  u32 i, j;

  fprintf(stderr, "%s[%d:%d]", NodeTypeName(node->type), node->start, node->end);

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
  case varNode:
    fprintf(stderr, " %s\n", SymbolName(node->data.value));
    break;
  case strNode:
    fprintf(stderr, " \"%s\"\n", SymbolName(RawVal(node->data.value)));
    break;
  default:
    fprintf(stderr, "\n");
    lines |= Bit(level);
    for (i = 0; i < VecCount(node->data.children); i++) {
      for (j = 0; j < level; j++) {
        if (lines & Bit(j)) {
          fprintf(stderr, "│ ");
        } else {
          fprintf(stderr, "  ");
        }
      }
      if (i < VecCount(node->data.children) - 1) {
        fprintf(stderr, "├ ");
      } else {
        lines &= ~Bit(level);
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
