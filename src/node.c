#include "node.h"
#include "types.h"
#include <univ/math.h>
#include <univ/symbol.h>

val MakeNode(i32 type, i32 start, i32 end, val value)
{
  val node = Tuple(5);
  TupleSet(node, 0, IntVal(type));
  TupleSet(node, 1, value);
  TupleSet(node, 2, IntVal(start));
  TupleSet(node, 3, IntVal(end));
  TupleSet(node, 4, 0);
  return node;
}

char *NodeTypeName(i32 type)
{
  switch (type) {
  case errNode:     return "error";
  case idNode:      return "id";
  case nilNode:     return "nil";
  case intNode:     return "num";
  case symNode:     return "sym";
  case strNode:     return "str";
  case listNode:    return "list";
  case tupleNode:   return "tuple";
  case doNode:      return "do";
  case ifNode:      return "if";
  case lambdaNode:  return "lambda";
  case notNode:     return "not";
  case lenNode:     return "len";
  case compNode:    return "comp";
  case negNode:     return "neg";
  case accessNode:  return "access";
  case sliceNode:   return "slice";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case bitandNode:  return "bitand";
  case subNode:     return "sub";
  case addNode:     return "add";
  case bitorNode:   return "bitor";
  case shiftNode:   return "shift";
  case ltNode:      return "lt";
  case gtNode:      return "gt";
  case pairNode:    return "pair";
  case eqNode:      return "eq";
  case andNode:     return "and";
  case orNode:      return "or";
  case callNode:    return "call";
  case letNode:     return "let";
  case defNode:     return "def";
  case importNode:  return "import";
  case moduleNode:  return "module";
  default:          assert(false);
  }
}

void PrintNodeLevel(val node, i32 level, u32 lines)
{
  i32 type = NodeType(node);
  i32 i, children;
  i32 valtype = NodeValType(node);

  printf("%s[%d:%d]", NodeTypeName(type), NodeStart(node), NodeEnd(node));
  if (valtype) {
    printf("<");
    PrintType(valtype);
    printf(">");
  }

  switch (type) {
  case nilNode:
    printf("\n");
    break;
  case idNode:
    printf(" %s\n", NodeText(node));
    break;
  case strNode:
    printf(" \"%s\"\n", NodeText(node));
    break;
  case symNode:
    printf(" :%s\n", NodeText(node));
    break;
  case intNode:
    printf(" %d\n", NodeInt(node));
    break;
  default:
    children = NodeChildren(node);
    printf("\n");
    lines |= Bit(level);
    while (children && IsPair(children)) {
      for (i = 0; i < level; i++) {
        if (lines & Bit(i)) {
          printf("│ ");
        } else {
          printf("  ");
        }
      }
      if (Tail(children)) {
        printf("├ ");
      } else {
        lines &= ~Bit(level);
        printf("└ ");
      }
      PrintNodeLevel(Head(children), level+1, lines);
      children = Tail(children);
    }
  }
}

void PrintNode(val node)
{
  PrintNodeLevel(node, 0, 0);
}
