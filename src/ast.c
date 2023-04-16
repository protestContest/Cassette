#include "ast.h"
#include "parse_syms.h"

#define DEBUG_ABSTRACT

Val AbstractNode(Parser *p, u32 sym, Val children)
{
  Val node = children;

  switch (sym) {
  case ParseSymProgram:
    node = MakePair(p->mem, SymbolFor("do"), Head(p->mem, children));
    break;

  case ParseSymStmts:
    if (ListLength(p->mem, children) == 1 && (IsNil(Head(p->mem, children)) || IsNil(Head(p->mem, Head(p->mem, children))))) {
      node = nil;
    } else {
      Val tail = MakePair(p->mem, ListAt(p->mem, children, 1), nil);
      node = ListAppend(p->mem, Head(p->mem, children), tail);
    }
    break;

  case ParseSymStmt:
    node = Head(p->mem, children);
    break;

  case ParseSymLet_stmt:
    if (IsList(p->mem, ListAt(p->mem, children, 2))) {
      node = MakeList(p->mem, 2, SymbolFor("let"), ListAt(p->mem, children, 2));
    } else {
      node = MakeList(p->mem, 2, SymbolFor("let"), MakePair(p->mem, ListAt(p->mem, children, 2), nil));
    }
    break;

  case ParseSymAssigns:
    if (IsList(p->mem, Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, Tail(p->mem, children))));
    } else {
      node = MakeList(p->mem, 2, ListAt(p->mem, children, 0), ListAt(p->mem, children, 3));
    }
    break;

  case ParseSymAssign: {
    node = MakePair(p->mem, ListAt(p->mem, children, 0), ListAt(p->mem, children, 2));
    break;
  }

  case ParseSymDef_block:
    node = MakeList(p->mem, 3, ListAt(p->mem, children, 0),
                               ListAt(p->mem, children, 2),
                               ListAt(p->mem, children, 5));
    break;

  case ParseSymParams:
    if (IsNil(Head(p->mem, children))) {
      node = Tail(p->mem, children);
    } else {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, children)));
    }
    break;

  case ParseSymCall:
    if (IsNil(Tail(p->mem, children))) {
      node = Head(p->mem, children);
      // node = children;
    // } else if (ListLength(p->mem, children) > 1) {
    } else {
      if (IsList(p->mem, Head(p->mem, children))) {
        node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, children));
      } else {
        node = children;
      }
    }
    // }
    break;

  case ParseSymMl_call:
    if (ListLength(p->mem, children) == 1) {
      node = children;
    } else if (IsPair(Head(p->mem, children))) {
      node = ListAppend(p->mem, Head(p->mem, children), Tail(p->mem, Tail(p->mem, children)));
    } else {
      node = MakeList(p->mem, 2, ListAt(p->mem, children, 0),
                                 ListAt(p->mem, children, 2));
    }
    break;

  case ParseSymLambda:
  case ParseSymLogic:
  case ParseSymEquals:
  case ParseSymPair:
  case ParseSymCompare:
  case ParseSymSum:
  case ParseSymProduct:
    if (ListLength(p->mem, children) > 1) {
      node = MakeList(p->mem, 3, ListAt(p->mem, children, 1),
                                 ListAt(p->mem, children, 0),
                                 ListAt(p->mem, children, 3));
    } else {
      node = Head(p->mem, children);
    }
    break;

  case ParseSymArg:
  case ParseSymPrimary:
  case ParseSymBlock:
    node = Head(p->mem, children);
    break;

  case ParseSymSymbol:
    node = MakeList(p->mem, 2, MakeSymbol(p->mem, "quote"), ListAt(p->mem, children, 1));
    break;

  case ParseSymDo_block:
    if (ListLength(p->mem, ListAt(p->mem, children, 1)) == 1) {
      node = Head(p->mem, ListAt(p->mem, children, 1));
    } else {
      node = ListAppend(p->mem, MakePair(p->mem, SymbolFor("do"), nil),
                                ListAt(p->mem, children, 1));
    }
    break;

  case ParseSymIf_block: {
    Val condition = ListAt(p->mem, children, 2);
    Val consequent = ListAt(p->mem, children, 5);
    if (ListLength(p->mem, consequent) == 1) {
      consequent = Head(p->mem, consequent);
    } else {
      consequent = MakePair(p->mem, SymbolFor("do"), consequent);
    }
    Val alternative = nil;
    if (ListLength(p->mem, children) > 8) {
      alternative = ListAt(p->mem, children, 8);
      if (ListLength(p->mem, alternative) == 1) {
        alternative = Head(p->mem, alternative);
      } else {
        alternative = MakePair(p->mem, SymbolFor("do"), alternative);
      }
    }
    node = MakeList(p->mem, 4, SymbolFor("if"), condition, consequent, alternative);
    break;
  }

  case ParseSymCond_block:
    node = MakePair(p->mem, SymbolFor("cond"), ListAt(p->mem, children, 2));
    break;

  case ParseSymClauses:
    node = ListAppend(p->mem, Head(p->mem, children),
                              Tail(p->mem, children));
    break;

  case ParseSymClause:
    node = MakePair(p->mem, ListAt(p->mem, children, 0),
                            ListAt(p->mem, children, 2));
    break;

  case ParseSymGroup:
    node = ListAt(p->mem, children, 1);
    break;

  case ParseSymList:
    node = MakePair(p->mem, MakeSymbol(p->mem, "list"), ListAt(p->mem, children, 1));
    break;

  case ParseSymItems:
    if (ListLength(p->mem, children) > 1) {
      node = ListAppend(p->mem, Head(p->mem, children), MakePair(p->mem, ListAt(p->mem, children, 1), nil));
    } else {
      node = nil;
    }
    break;

  case ParseSymEntries:
    if (ListLength(p->mem, children) > 1) {
      node = ListAppend(p->mem, Head(p->mem, children), ListAt(p->mem, children, 1));
    } else {
      node = nil;
    }
    break;

  case ParseSymTuple:
    node = MakePair(p->mem, MakeSymbol(p->mem, "tuple"), ListAt(p->mem, children, 1));
    break;

  case ParseSymDict: {
    node = MakePair(p->mem, MakeSymbol(p->mem, "dict"), ListAt(p->mem, children, 1));
    break;
  }

  case ParseSymEntry: {
    Val key = ListAt(p->mem, children, 0);
    key = MakeList(p->mem, 2, MakeSymbol(p->mem, "quote"), key);
    node = MakeList(p->mem, 2, key, ListAt(p->mem, children, 3));
    break;
  }
  }

#ifdef DEBUG_ABSTRACT
  Print(GrammarSymbolName(sym));
  Print(": ");
  PrintVal(p->mem, children);


  Print(" -> ");
  PrintVal(p->mem, node);
  Print("\n");
#endif

  return node;
}