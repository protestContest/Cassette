#include "ast.h"
#include "parse.h"
#include "parse_syms.h"

Val MakeTerm(Val value, Val line, Val col, Mem *mem)
{
  return
    MakePair(mem, MakeSymbol(mem, "term"),
    MakePair(mem, value,
    MakePair(mem, line,
    MakePair(mem, col, nil))));
}

bool IsTerm(Val term, Mem *mem)
{
  return IsTagged(mem, term, "term");
}

bool IsTaggedTerm(Mem *mem, Val list, Val tag)
{
  if (!IsList(mem, list)) return false;
  Val term = Head(mem, list);
  if (!IsTerm(term, mem)) return false;
  return Eq(TermVal(term, mem), tag);
}

Val TermVal(Val term, Mem *mem)
{
  if (IsTerm(term, mem)) {
    return ListAt(mem, term, 1);
  } else {
    return term;
  }
}

Val TermLine(Val term, Mem *mem)
{
  return ListAt(mem, term, 2);
}

Val TermCol(Val term, Mem *mem)
{
  return ListAt(mem, term, 3);
}

void PrintTerm(Val term, Mem *mem)
{
  if (IsTagged(mem, term, "term")) {
    PrintVal(mem, TermVal(term, mem));
    Print("[");
    PrintVal(mem, TermLine(term, mem));
    Print(":");
    PrintVal(mem, TermCol(term, mem));
    Print("]");
  } else if (IsList(mem, term)) {
    PrintTerms(term, mem);
  } else {
    PrintVal(mem, term);
  }
}

void PrintTerms(Val children, Mem *mem)
{
  Print("[ ");
  while (!IsNil(children)) {
    PrintTerm(Head(mem, children), mem);
    Print(" ");
    children = Tail(mem, children);
  }
  Print(" ]");
}

static bool IsInfix(Val sym)
{
  char *infixes[] = {"+", "-", "*", "/", "<", "<=", ">", ">=", "|", "==", "!=", "and", "or", "->"};
  for (u32 i = 0; i < ArrayCount(infixes); i++) {
    if (Eq(sym, SymbolFor(infixes[i]))) return true;
  }
  return false;
}

static Val ParseProgram(Val children, Mem *mem)
{
  Val stmts = Head(mem, children);
  return MakePair(mem, SymbolFor("do"), stmts);
}

static Val ParseSequence(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 1) {
    return ListAppend(mem, ListAt(mem, children, 0), ListAt(mem, children, 1));
  } else {
    return children;
  }
}

static Val ParseStmt(Val children, Mem *mem)
{
  Val stmt = Head(mem, children);
  if (ListLength(mem, stmt) == 1) {
    return Head(mem, stmt);
  } else {
    return stmt;
  }
}

static Val ParseLetStmt(Val children, Mem *mem)
{
  return MakePair(mem, SymbolFor("let"), ListAt(mem, children, 1));
}

static Val ParseAssigns(Val children, Mem *mem)
{
  PrintVal(mem, children);
  Print("\n");
  if (ListLength(mem, children) > 1) {
    return ListAppend(mem, ListAt(mem, children, 0), ListAt(mem, children, 2));
  } else {
    return children;
  }
}

static Val ParseAssign(Val children, Mem *mem)
{
  Val value = ListAt(mem, children, 2);
  if (IsNil(Tail(mem, value))) {
    value = Head(mem, value);
  }
  return
    MakePair(mem, ListAt(mem, children, 0),
    MakePair(mem, value, nil));
}

static Val ParseDefStmt(Val children, Mem *mem)
{
  Val params = ListAt(mem, children, 2);
  Val var = Head(mem, params);
  params = Tail(mem, params);
  Val body = ListAt(mem, children, 4);

  Val lambda =
    MakePair(mem, SymbolFor("->"),
    MakePair(mem, params,
    MakePair(mem, body, nil)));

  return
    MakePair(mem, SymbolFor("let"),
    MakePair(mem,
      MakePair(mem, var,
      MakePair(mem, lambda, nil)), nil));
}

static Val ParseImportStmt(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 2) {
    Val filename = ListAt(mem, children, 1);
    Val var = ListAt(mem, children, 3);
    return
      MakePair(mem, SymbolFor("import"),
      MakePair(mem, filename,
      MakePair(mem, var, nil)));
  } else {
    return MakePair(mem, SymbolFor("import"), Tail(mem, children));
  }
}

static Val ParseInfix(Val children, Mem *mem)
{
  if (IsNil(Tail(mem, children))) {
    return Head(mem, children);
  } else {
    Val op = ListAt(mem, children, 1);
    if (IsTerm(op, mem)) op = TermVal(op, mem);
    return
      MakePair(mem, op,
      MakePair(mem, ListAt(mem, children, 0),
      MakePair(mem, ListAt(mem, children, 2), nil)));
  }
}

static Val ParseLambda(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 3) {
    return
      MakePair(mem, SymbolFor("->"),
      MakePair(mem, nil,
      MakePair(mem, ListAt(mem, children, 3), nil)));
  }
  return ParseInfix(children, mem);
}

static Val ParseAccess(Val children, Mem *mem)
{
  Val dict = ListAt(mem, children, 0);
  Val key =
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, ListAt(mem, children, 2), nil));

  return
    MakePair(mem, SymbolFor("."),
    MakePair(mem, dict,
    MakePair(mem, key, nil)));
}

static Val ParseSimple(Val children, Mem *mem)
{
  return Head(mem, children);
}

static Val ParseSymbol(Val children, Mem *mem)
{
  Val name = TermVal(ListAt(mem, children, 1), mem);
  return
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, name, nil));
}

static Val ParseDoBlock(Val children, Mem *mem)
{
  return MakePair(mem, SymbolFor("do"), ListAt(mem, children, 1));
}

static Val ParseIfBlock(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 3) {
    return MakePair(mem, SymbolFor("if"), ListAppend(mem, Tail(mem, children), nil));
  }

  Val predicate = ListAt(mem, children, 1);
  Val consequent = ListAt(mem, children, 3);
  Val alternative = ListAt(mem, children, 5);

  if (ListLength(mem, consequent) == 1) {
    consequent = Head(mem, consequent);
  } else {
    consequent = MakePair(mem, SymbolFor("do"), consequent);
  }

  if (ListLength(mem, alternative) == 1) {
    alternative = Head(mem, alternative);
  } else {
    alternative = MakePair(mem, SymbolFor("do"), alternative);
  }

  return
    MakePair(mem, SymbolFor("if"),
    MakePair(mem, predicate,
    MakePair(mem, consequent,
    MakePair(mem, alternative, nil))));
}

static Val CondToIf(Val exp, Mem *mem)
{
  if (IsNil(exp)) return nil;

  Val clause = Head(mem, exp);
  Val predicate = ListAt(mem, clause, 0);
  Val consequent = ListAt(mem, clause, 1);
  return
    MakePair(mem, SymbolFor("if"),
    MakePair(mem, predicate,
    MakePair(mem, consequent,
    MakePair(mem, CondToIf(Tail(mem, exp), mem), nil))));
}

static Val ParseCondBlock(Val children, Mem *mem)
{
  return CondToIf(ListAt(mem, children, 2), mem);
}

static Val ParseClause(Val children, Mem *mem)
{
  Val predicate = ListAt(mem, children, 0);
  if (ListLength(mem, predicate) == 1) predicate = Head(mem, predicate);
  Val consequent = ListAt(mem, children, 2);
  if (ListLength(mem, consequent) == 1) consequent = Head(mem, consequent);
  return
    MakePair(mem, predicate,
    MakePair(mem, consequent, nil));
}

static Val ParseGroup(Val children, Mem *mem)
{
  Val group = ListAt(mem, children, 1);

  if (ListLength(mem, group) == 1) {
    // maybe an infix application
    Val op = Head(mem, group);
    if (IsTerm(Head(mem, op), mem) && IsInfix(TermVal(Head(mem, op), mem))) {
      // infix application, don't call the result
      return op;
    }
  }
  return group;
}

static Val ParseCollection(Val children, Mem *mem)
{
  Val symbol = ListAt(mem, children, 0);
  Val items = ListAt(mem, children, 1);
  Val update = ListAt(mem, children, 2);

  if (IsTaggedTerm(mem, update, SymbolFor("|"))) {
    // reverse the list here so it's faster to prepend items at runtime
    return ListAppend(mem, update, ReverseOnto(mem, items, nil));
  } else {
    return MakePair(mem, symbol, items);
  }
}

static Val ParseEntry(Val children, Mem *mem)
{
  Val key =
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, ListAt(mem, children, 0), nil));
  Val value = ListAt(mem, children, 2);

  return
    MakePair(mem, key,
    MakePair(mem, value, nil));
}

static Val ParseUpdate(Val children, Mem *mem)
{
  return ListAt(mem, children, 2);
}

Val ParseNode(u32 sym, Val children, Mem *mem)
{
  if (IsNil(children)) return nil;

  switch (sym) {
  case ParseSymProgram:     return ParseProgram(children, mem);
  case ParseSymStmts:       return ParseSequence(children, mem);
  case ParseSymStmt:        return ParseStmt(children, mem);
  case ParseSymLetStmt:     return ParseLetStmt(children, mem);
  case ParseSymAssigns:     return ParseAssigns(children, mem);
  case ParseSymAssign:      return ParseAssign(children, mem);
  case ParseSymDefStmt:     return ParseDefStmt(children, mem);
  case ParseSymParams:      return ParseSequence(children, mem);
  case ParseSymImportStmt:  return ParseImportStmt(children, mem);
  case ParseSymCall:        return ParseSequence(children, mem);
  case ParseSymMlCall:      return ParseSequence(children, mem);
  case ParseSymArg:         return ParseSimple(children, mem);
  case ParseSymLambda:      return ParseLambda(children, mem);
  case ParseSymLogic:       return ParseInfix(children, mem);
  case ParseSymEquals:      return ParseInfix(children, mem);
  case ParseSymCompare:     return ParseInfix(children, mem);
  case ParseSymSum:         return ParseInfix(children, mem);
  case ParseSymProduct:     return ParseInfix(children, mem);
  case ParseSymBlock:       return ParseSimple(children, mem);
  case ParseSymDoBlock:     return ParseDoBlock(children, mem);
  case ParseSymIfBlock:     return ParseIfBlock(children, mem);
  case ParseSymCondBlock:   return ParseCondBlock(children, mem);
  case ParseSymClauses:     return ParseSequence(children, mem);
  case ParseSymClause:      return ParseClause(children, mem);
  case ParseSymPrimary:     return ParseSimple(children, mem);
  case ParseSymAccess:      return ParseAccess(children, mem);
  case ParseSymLiteral:     return ParseSimple(children, mem);
  case ParseSymSymbol:      return ParseSymbol(children, mem);
  case ParseSymGroup:       return ParseGroup(children, mem);
  case ParseSymList:        return ParseCollection(children, mem);
  case ParseSymItems:       return ParseSequence(children, mem);
  case ParseSymTuple:       return ParseCollection(children, mem);
  case ParseSymDict:        return ParseCollection(children, mem);
  case ParseSymEntries:     return ParseSequence(children, mem);
  case ParseSymEntry:       return ParseEntry(children, mem);
  case ParseSymOptComma:    return nil;
  default:                  return children;
  }
}

static void Indent(u32 level, u32 lines)
{
  if (level == 0) return;

  for (u32 i = 0; i < level - 1; i++) {
    if ((lines >> (level - i - 1)) & 1) {
      Print("│ ");
    } else {
      Print("  ");
    }
  }
}

static void PrintASTNode(Val node, u32 indent, u32 lines, Mem *mem)
{
  if (IsNil(node)) {
    Print("╴");
    Print("nil\n");
  } else if (IsList(mem, node) && !IsTerm(node, mem)) {
    Print("┐\n");
    u32 next_lines = (lines << 1) + 1;
    while (!IsNil(node)) {
      bool is_last = IsNil(Tail(mem, node)) || !IsList(mem, Tail(mem, node));
      if (is_last) {
        next_lines -= 1;
      }

      Indent(indent + 1, next_lines);
      if (is_last) {
        Print("└─");
      } else {
        Print("├─");
      }
      PrintASTNode(Head(mem, node), indent + 1, next_lines, mem);
      node = Tail(mem, node);
    }
  } else if (IsPair(node) && !IsTerm(node, mem)) {
    Print("╴");
    PrintVal(mem, Head(mem, node));
    Print(" | ");
    PrintVal(mem, Tail(mem, node));
    Print("\n");
  } else {
    Print("╴");
    if (IsTerm(node, mem)) {
      PrintTerm(node, mem);
    } else {
      PrintVal(mem, node);
    }
    Print("\n");
  }
}

void PrintAST(Val ast, Mem *mem)
{
  if (IsNil(ast)) return;
  PrintASTNode(ast, 0, 0, mem);
}
