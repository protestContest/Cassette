#include "node.h"
#include "types.h"
#include "univ/math.h"
#include "univ/str.h"

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
  case nilNode:     return "nil";
  case idNode:      return "id";
  case intNode:     return "num";
  case symNode:     return "sym";
  case strNode:     return "str";
  case listNode:    return "list";
  case doNode:      return "do";
  case ifNode:      return "if";
  case andNode:     return "and";
  case orNode:      return "or";
  case callNode:    return "call";
  case lambdaNode:  return "lambda";
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

void PrintError(char *prefix, val node, char *source)
{
  i32 nodeStart = NodeStart(node);
  i32 nodeLen = NodeEnd(node) - nodeStart;
  i32 col = 0, i;
  i32 line = 0;
  if (!node) return;
  if (prefix) {
    fprintf(stderr, "%s%s: %s\n", ANSIRed, prefix, ErrorMsg(node));
  } else {
    fprintf(stderr, "%s%s\n", ANSIRed, ErrorMsg(node));
  }
  if (source) {
    char *start = source + nodeStart;
    char *end = source + nodeStart;
    while (start > source && !IsNewline(start[-1])) {
      col++;
      start--;
    }
    while (*end && !IsNewline(*end)) end++;
    for (i = 0; i < nodeStart; i++) {
      if (IsNewline(source[i])) line++;
    }
    fprintf(stderr, "%3d| %*.*s\n",
        line+1, (i32)(end-start), (i32)(end-start), start);
    fprintf(stderr, "     ");
    for (i = 0; i < col; i++) fprintf(stderr, " ");
    for (i = 0; i < nodeLen; i++) fprintf(stderr, "^");
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "%s", ANSINormal);
}
