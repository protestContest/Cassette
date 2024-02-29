#include "ast.h"
#include "univ/system.h"
#include "univ/math.h"
#include <stdio.h>

Node *MakeTerminal(NodeType type, u32 position, void *value)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  node->expr.value = value;
  return node;
}

Node *MakeNode(NodeType type, u32 position)
{
  Node *node = Alloc(sizeof(Node));
  node->type = type;
  node->pos = position;
  InitVec(&node->expr.children, sizeof(Node*), 2);
  return node;
}

Node *MakeIntNode(u32 position, i64 value)
{
  Node *node = Alloc(sizeof(Node));
  node->type = IntNode;
  node->pos = position;
  node->expr.intval = value;
  return node;
}

Node *MakeFloatNode(u32 position, double value)
{
  Node *node = Alloc(sizeof(Node));
  node->type = FloatNode;
  node->pos = position;
  node->expr.floatval = value;
  return node;
}

bool IsTerminal(Node *node)
{
  switch (node->type) {
  case NilNode:
  case IDNode:
  case IntNode:
  case FloatNode:
  case StringNode:
  case SymbolNode:
    return true;
  default:
    return false;
  }
}

void SetNodeType(Node *node, NodeType type)
{
  node->type = type;
}

void FreeNode(Node *node)
{
  if (!IsTerminal(node)) {
    DestroyVec(&node->expr.children);
  }

  Free(node);
}

void FreeAST(Node *node)
{
  if (!IsTerminal(node)) {
    u32 i;
    for (i = 0; i < NumNodeChildren(node); i++) {
      FreeNode(NodeChild(node, i));
    }
  }

  FreeNode(node);
}

static char *NodeTypeName(NodeType type)
{
  switch (type) {
  case ModuleNode: return "Module";
  case ImportNode: return "Import";
  case ExportNode: return "Export";
  case SetNode: return "Set";
  case LetNode: return "Let";
  case DefNode: return "Def";
  case SymbolNode: return "Symbol";
  case DoNode: return "Do";
  case LambdaNode: return "Lambda";
  case CallNode: return "Call";
  case NilNode: return "Nil";
  case IfNode: return "If";
  case ListNode: return "List";
  case MapNode: return "Map";
  case TupleNode: return "Tuple";
  case IDNode: return "ID";
  case IntNode: return "Int";
  case FloatNode: return "Float";
  case StringNode: return "String";
  case AndNode: return "And";
  case OrNode: return "Or";
  case PairNode: return "Pair";
  case AccessNode: return "Access";
  case AddNode: return "Add";
  case SubNode: return "Sub";
  case MulNode: return "Mul";
  case DivNode: return "Div";
  case RemNode: return "Rem";
  case LtNode: return "Lt";
  case LtEqNode: return "LtEq";
  case InNode: return "In";
  case LenNode: return "Len";
  case EqNode: return "Eq";
  case NotEqNode: return "NotEq";
  case GtNode: return "Gt";
  case GtEqNode: return "GtEq";
  case NotNode: return "Not";
  }
}

static void Indent(u32 level, u32 lines)
{
  u32 i;
  for (i = 0; i < level; i++) {
    if (lines & Bit(i)) printf("│ ");
    else printf("  ");
  }
}

static void PrintASTNode(Node *node, u32 level, u32 lines)
{
  NodeType type = node->type;
  char *name = NodeTypeName(type);
  u32 i;

  printf("%s:%d", name, node->pos);

  if (type == ModuleNode) {
    char *name = NodeValue(NodeChild(node, 3));
    Node *body = NodeChild(node, 2);

    printf(" %s\n", name);
    printf("└╴");
    PrintASTNode(body, level+1, lines);
  } else if (type == DoNode) {
    Node *assigns = NodeChild(node, 0);
    Node *stmts = NodeChild(node, 1);

    printf(" Assigns: [");
    for (i = 0; i < NumNodeChildren(assigns); i++) {
      char *assign = NodeValue(NodeChild(assigns, i));
      printf("%s", assign);
      if (i < NumNodeChildren(assigns) - 1) printf(", ");
    }
    printf("]\n");

    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(stmts); i++) {
      if (i == NumNodeChildren(stmts) - 1) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(NodeChild(stmts, i), level+1, lines);
    }
  } else if (type == LetNode || type == DefNode) {
    char *var = NodeValue(NodeChild(node, 0));
    printf(" %s\n", var);
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines);
  } else if (type == ImportNode) {
    char *mod = NodeValue(NodeChild(node, 0));
    Node *alias = NodeChild(node, 1);
    printf(" %s as ", mod);
    if (alias->type == NilNode) {
      printf("*\n");
    } else {
      printf("%s\n", (char*)NodeValue(alias));
    }
  } else if (type == LambdaNode) {
    Node *params = NodeChild(node, 0);
    printf(" (");
    for (i = 0; i < NumNodeChildren(params); i++) {
      char *param = NodeValue(NodeChild(params, i));
      printf("%s", param);
      if (i < NumNodeChildren(params) - 1) printf(", ");
    }
    printf(")\n");
    Indent(level, lines);
    printf("└╴");
    PrintASTNode(NodeChild(node, 1), level+1, lines);
  } else if (type == IntNode) {
    printf(" %lld\n", node->expr.intval);
  } else if (type == FloatNode) {
    printf(" %f\n", node->expr.floatval);
  } else if (type == IDNode) {
    printf(" %s\n", (char*)NodeValue(node));
  } else if (type == SymbolNode) {
    printf(" :%s\n", (char*)NodeValue(node));
  } else if (type == StringNode) {
    printf(" \"%s\"\n", (char*)NodeValue(node));
  } else if (type == MapNode) {
    printf("\n");
    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(node); i += 2) {
      Node *key = NodeChild(node, i);
      Node *value = NodeChild(node, i+1);

      if (i == NumNodeChildren(node) - 2) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      printf("%s: ", (char*)NodeValue(key));
      PrintASTNode(value, level+1, lines);
    }
  } else if (type == NilNode) {
    printf("\n");
  } else {
    printf("\n");
    lines |= Bit(level);
    for (i = 0; i < NumNodeChildren(node); i++) {
      if (i == NumNodeChildren(node) - 1) {
        lines &= ~Bit(level);
        Indent(level, lines);
        printf("└╴");
      } else {
        Indent(level, lines);
        printf("├╴");
      }

      PrintASTNode(NodeChild(node, i), level+1, lines);
    }
  }
}

void PrintAST(Node *ast)
{
  PrintASTNode(ast, 0, 0);
}
