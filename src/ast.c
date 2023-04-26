#include "ast.h"
#include "parse_syms.h"

static bool IsInfix(Val sym)
{
  char *infixes[] = {"+", "-", "*", "/", "<", "<=", ">", ">=", "|", "==", "!=", "and", "or", "->"};
  for (u32 i = 0; i < ArrayCount(infixes); i++) {
    if (Eq(sym, SymbolFor(infixes[i]))) return true;
  }
  return false;
}

Val AbstractNode(Parser *p, u32 sym, Val children)
{
  Mem *mem = p->mem;
  Val node = children;

  switch (sym) {
  case ParseSymProgram:
    node = MakePair(mem, SymbolFor("do"), Head(mem, children));
    break;

  case ParseSymStmts:
    if (ListLength(mem, children) == 1 && (IsNil(Head(mem, children)) || IsNil(Head(mem, Head(mem, children))))) {
      node = nil;
    } else {
      Val tail = MakePair(mem, ListAt(mem, children, 1), nil);
      node = ListConcat(mem, Head(mem, children), tail);
    }
    break;

  case ParseSymStmt:
    node = Head(mem, children);
    if (IsNil(Tail(mem, node))) {
      node = Head(mem, node);
    }
    break;

  case ParseSymLet_stmt:
    node = MakePair(mem, SymbolFor("let"), ListAt(mem, children, 2));
    break;

  case ParseSymAssigns:
    if (IsNil(Tail(mem, children))) {
      node = Head(mem, children);
    } else {
      node = ListConcat(mem, ListAt(mem, children, 0), ListAt(mem, children, 3));
    }
    break;

  case ParseSymAssign: {
    Val value = ListAt(mem, children, 2);
    if (IsNil(Tail(mem, value))) {
      value = Head(mem, value);
    }
    node = MakeList(mem, 2, ListAt(mem, children, 0), value);
    break;
  }

  case ParseSymDef_stmt:
    if (ListLength(mem, children) > 3) {
      Val ids = ListAt(mem, children, 2);
      Val var = Head(mem, ids);
      Val params = Tail(mem, ids);
      Val body = ListAt(mem, children, 4);
      Val lambda = MakeList(mem, 3, SymbolFor("->"), params, body);
      node = MakeList(mem, 3, SymbolFor("let"), var, lambda);
    }
    break;

  case ParseSymParams:
    if (!IsNil(Tail(mem, children))) {
      node = ListConcat(mem, Head(mem, children), Tail(mem, children));
    }
    break;

  case ParseSymCall:
    if (!IsNil(Tail(mem, children))) {
      node = ListConcat(mem, Head(mem, children), Tail(mem, children));
    }
    break;

  case ParseSymMl_call:
    if (!IsNil(Tail(mem, children))) {
      node = ListConcat(mem, Head(mem, children), ListFrom(mem, children, 2));
    }
    break;

  case ParseSymLambda:
  case ParseSymLogic:
  case ParseSymEquals:
  case ParseSymCompare:
  case ParseSymSum:
  case ParseSymProduct:
    if (IsNil(Tail(mem, children))) {
      node = Head(mem, children);
    } else {
      node = MakeList(mem, 3, ListAt(mem, children, 1),
                              ListAt(mem, children, 0),
                              ListAt(mem, children, 3));
    }
    break;

  case ParseSymArg:
  case ParseSymPrimary:
  case ParseSymBlock:
  case ParseSymLiteral:
    if (IsNil(Tail(mem, children))) {
      node = Head(mem, children);
    }
    break;

  case ParseSymAccess:
    node = MakeList(mem, 3, ListAt(mem, children, 1),
                            ListAt(mem, children, 0),
                            ListAt(mem, children, 2));
    break;

  case ParseSymSymbol:
    node = MakeList(mem, 2, MakeSymbol(mem, ":"), ListAt(mem, children, 1));
    break;

  case ParseSymDo_block:
    node = MakePair(mem, SymbolFor("do"), ListConcat(mem, ListAt(mem, children, 1), ListAt(mem, children, 2)));
    break;

  case ParseSymIf_block:
    if (ListLength(mem, children) > 3) {
      Val condition = ListAt(mem, children, 1);
      Val consequent = MakePair(mem, SymbolFor("do"), ListAt(mem, children, 3));
      Val alternative = MakePair(mem, SymbolFor("do"), ListAt(mem, children, 6));
      node = MakeList(mem, 4, SymbolFor("if"), condition, consequent, alternative);
    } else {
      node = ListAppend(mem, children, nil);
    }
    break;

  case ParseSymCond_block:
    node = MakePair(mem, SymbolFor("cond"), ListAt(mem, children, 2));
    break;

  case ParseSymClauses:
    node = ListConcat(mem, Head(mem, children), Tail(mem, children));
    break;

  case ParseSymClause: {
    Val condition = ListAt(mem, children, 0);
    if (IsNil(Tail(mem, condition))) condition = Head(mem, condition);
    Val consequent = ListAt(mem, children, 3);
    if (IsNil(Tail(mem, consequent))) consequent = Head(mem, consequent);
    node = MakeList(mem, 2, condition, consequent);
    break;
  }

  case ParseSymGroup:
    node = ListAt(mem, children, 1);
    if (IsPair(Head(mem, node)) && IsInfix(Head(mem, Head(mem, node)))) {
      node = Head(mem, node);
    }
    break;

  case ParseSymList:
    if (IsNil(ListAt(mem, children, 2))) {
      node = MakePair(mem, MakeSymbol(mem, "["), ListAt(mem, children, 1));
    } else if (ListLength(mem, ListAt(mem, children, 1)) == 1) {
      node = MakeList(mem, 3, MakeSymbol(mem, "|"),
                              Head(mem, ListAt(mem, children, 1)),
                              ListAt(mem, children, 2));
    } else {
      node = MakeList(mem, 3, MakeSymbol(mem, "[+"),
                              MakePair(mem, MakeSymbol(mem, "["), ListAt(mem, children, 1)),
                              ListAt(mem, children, 2));
    }
    break;

  case ParseSymItems:
    if (IsNil(Tail(mem, children))) {
      node = nil;
    } else {
      node = ListAppend(mem, ListAt(mem, children, 0), ListAt(mem, children, 1));
    }
    break;

  case ParseSymEntries:
    if (IsNil(Tail(mem, children))) {
      node = nil;
    } else {
      node = ListConcat(mem, Head(mem, children), ListAt(mem, children, 1));
    }
    break;

  case ParseSymTuple:
    node = MakePair(mem, MakeSymbol(mem, "#["), ListAt(mem, children, 1));
    break;

  case ParseSymDict:
    node = MakePair(mem, MakeSymbol(mem, "{"), ListAt(mem, children, 1));
    if (!IsNil(ListAt(mem, children, 2))) {
      node = MakeList(mem, 3, MakeSymbol(mem, "{|"),
                              ListAt(mem, children, 2),
                              node);
    }
    break;

  case ParseSymEntry:
    node = MakeList(mem, 2, MakeList(mem, 2, MakeSymbol(mem, ":"), Head(mem, children)),
                            ListAt(mem, children, 3));
    break;

  case ParseSymUpdate:
    node = ListAt(mem, children, 2);
    break;

  case ParseSymNl:
    node = nil;
    break;
  }

#ifdef DEBUG_AST
  Print(GrammarSymbolName(sym));
  Print(": ");
  PrintVal(mem, children);


  Print(" -> ");
  PrintVal(mem, node);
  Print("\n");
#endif

  return node;
}

static void Indent(u32 level, u32 lines)
{
  if (level == 0) return;

  for (u32 i = 0; i < level - 1; i++) {
    if ((lines >> (level - i - 1)) & 1) {
      Print("│");
    } else {
      Print(" ");
    }
  }
}

static void PrintASTNode(Val node, u32 indent, u32 lines, Mem *mem)
{
  if (IsNil(node)) {
    Print("╴");
    Print("nil\n");
  } else if (IsList(mem, node)) {
    Print("┬");
    u32 next_lines = (lines << 1);
    bool is_last = IsNil(Tail(mem, node)) || !IsList(mem, Tail(mem, node));
    next_lines = (lines << 1);
    if (!is_last) {
      next_lines += 1;
    }

    PrintASTNode(Head(mem, node), indent + 1, next_lines, mem);
    node = Tail(mem, node);
    while (!IsNil(node)) {
      bool is_last = IsNil(Tail(mem, node)) || !IsList(mem, Tail(mem, node));
      next_lines = (lines << 1);
      if (!is_last) {
        next_lines += 1;
      }

      Indent(indent + 1, next_lines);
      if (is_last) {
        Print("└");
      } else {
        Print("├");
      }
      PrintASTNode(Head(mem, node), indent + 1, next_lines, mem);
      node = Tail(mem, node);
    }
  } else if (IsPair(node)) {
    Print("╴");
    PrintVal(mem, Head(mem, node));
    Print(" | ");
    PrintVal(mem, Tail(mem, node));
    Print("\n");
  } else {
    Print("╴");
    PrintVal(mem, node);
    Print("\n");
  }
}

void PrintAST(Val ast, Mem *mem)
{
  if (IsNil(ast)) return;
  PrintASTNode(ast, 0, 0, mem);
}
