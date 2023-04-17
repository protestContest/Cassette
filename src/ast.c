#include "ast.h"
#include "parse_syms.h"

#define DEBUG_ABSTRACT

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
      node = ListAppend(mem, Head(mem, children), tail);
    }
    break;

  case ParseSymStmt:
    node = Head(mem, children);
    break;

  case ParseSymLet_stmt:
    node = MakePair(mem, SymbolFor("let"), ListAt(mem, children, 2));
    break;

  case ParseSymAssigns:
    if (IsNil(Tail(mem, children))) {
      node = children;
    } else {
      Val new_assign = ListFrom(mem, children, 3);
      node = ListAppend(mem, Head(mem, children), new_assign);
    }
    break;

  case ParseSymAssign: {
    node = MakePair(mem, ListAt(mem, children, 0), ListAt(mem, children, 2));
    break;
  }

  case ParseSymImport_stmt:
    if (ListLength(mem, children) > 2) {
      node = MakeList(mem, 3, ListAt(mem, children, 0),
                              ListAt(mem, children, 1),
                              ListAt(mem, children, 3));
    }
    break;

  case ParseSymDef_block:
    node = MakeList(mem, 3, ListAt(mem, children, 0),
                               ListAt(mem, children, 2),
                               ListAt(mem, children, 5));
    break;

  case ParseSymParams:
    if (IsNil(Head(mem, children))) {
      node = Tail(mem, children);
    } else {
      node = ListAppend(mem, Head(mem, children), ListFrom(mem, children, 2));
    }
    break;

  case ParseSymCall:
    if (!IsNil(Tail(mem, children))) {
      node = ListAppend(mem, Head(mem, children), Tail(mem, children));
    }
    // if (IsNil(Tail(mem, children))) {
    //   node = Head(mem, children);
    //   // node = children;
    // // } else if (ListLength(mem, children) > 1) {
    // } else {
    //   if (IsList(mem, Head(mem, children))) {
    //     node = ListAppend(mem, Head(mem, children), Tail(mem, children));
    //   } else {
    //     node = children;
    //   }
    // }
    // // }
    break;

  case ParseSymMl_call:
    if (ListLength(mem, children) == 1) {
      node = children;
    } else if (IsPair(Head(mem, children))) {
      node = ListAppend(mem, Head(mem, children), ListFrom(mem, children, 2));
    } else {
      node = MakeList(mem, 2, ListAt(mem, children, 0),
                                 ListAt(mem, children, 2));
    }
    break;

  case ParseSymLambda:
  case ParseSymLogic:
  case ParseSymEquals:
  case ParseSymPair:
  case ParseSymCompare:
  case ParseSymSum:
  case ParseSymProduct:
    if (ListLength(mem, children) > 1) {
      node = MakeList(mem, 3, ListAt(mem, children, 1),
                                 ListAt(mem, children, 0),
                                 ListAt(mem, children, 3));
    } else {
      node = Head(mem, children);
    }
    break;

  case ParseSymArg:
  case ParseSymPrimary:
  case ParseSymBlock:
    node = Head(mem, children);
    break;

  case ParseSymAccess:
    node = MakeList(mem, 3, ListAt(mem, children, 1),
                               ListAt(mem, children, 0),
                               ListAt(mem, children, 2));
    break;

  case ParseSymSymbol:
    node = MakeList(mem, 2, MakeSymbol(mem, "quote"), ListAt(mem, children, 1));
    break;

  case ParseSymDo_block:
    if (ListLength(mem, ListAt(mem, children, 1)) == 1) {
      node = Head(mem, ListAt(mem, children, 1));
    } else {
      node = ListAppend(mem, MakePair(mem, SymbolFor("do"), nil),
                                ListAt(mem, children, 1));
    }
    break;

  case ParseSymIf_block: {
    Val condition = ListAt(mem, children, 2);
    Val consequent = ListAt(mem, children, 5);
    if (ListLength(mem, consequent) == 1) {
      consequent = Head(mem, consequent);
    } else {
      consequent = MakePair(mem, SymbolFor("do"), consequent);
    }
    Val alternative = nil;
    if (ListLength(mem, children) > 8) {
      alternative = ListAt(mem, children, 8);
      if (ListLength(mem, alternative) == 1) {
        alternative = Head(mem, alternative);
      } else {
        alternative = MakePair(mem, SymbolFor("do"), alternative);
      }
    }
    node = MakeList(mem, 4, SymbolFor("if"), condition, consequent, alternative);
    break;
  }

  case ParseSymCond_block:
    node = MakePair(mem, SymbolFor("cond"), ListAt(mem, children, 2));
    break;

  case ParseSymClauses:
    node = ListAppend(mem, Head(mem, children),
                              Tail(mem, children));
    break;

  case ParseSymClause:
    node = MakePair(mem, ListAt(mem, children, 0),
                            ListAt(mem, children, 2));
    break;

  case ParseSymGroup:
    node = ListAt(mem, children, 1);
    break;

  case ParseSymList:
    node = MakePair(mem, MakeSymbol(mem, "list"), ListAt(mem, children, 1));
    break;

  case ParseSymItems:
    if (ListLength(mem, children) > 1) {
      node = ListAppend(mem, Head(mem, children), MakePair(mem, ListAt(mem, children, 1), nil));
    } else {
      node = nil;
    }
    break;

  case ParseSymEntries:
    if (ListLength(mem, children) > 1) {
      node = ListAppend(mem, Head(mem, children), ListAt(mem, children, 1));
    } else {
      node = nil;
    }
    break;

  case ParseSymTuple:
    node = MakePair(mem, MakeSymbol(mem, "tuple"), ListAt(mem, children, 1));
    break;

  case ParseSymDict: {
    node = MakePair(mem, MakeSymbol(mem, "dict"), ListAt(mem, children, 1));
    break;
  }

  case ParseSymEntry: {
    Val key = ListAt(mem, children, 0);
    key = MakeList(mem, 2, MakeSymbol(mem, "quote"), key);
    node = MakeList(mem, 2, key, ListAt(mem, children, 3));
    break;
  }
  }

#ifdef DEBUG_ABSTRACT
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
