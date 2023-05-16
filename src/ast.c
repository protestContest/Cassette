#include "ast.h"
#include "parse.h"
#include "parse_syms.h"
#include "print.h"

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
  if (ListLength(mem, stmts) == 1) {
    return Head(mem, stmts);
  } else {
    return MakePair(mem, SymbolFor("do"), stmts);
  }
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
    return ListConcat(mem, ListAt(mem, children, 0), ListAt(mem, children, 2));
  } else {
    return Head(mem, children);
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
    MakePair(mem, var,
    MakePair(mem, lambda, nil)));
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
  Val map = ListAt(mem, children, 0);
  Val key =
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, ListAt(mem, children, 2), nil));

  return
    MakePair(mem, SymbolFor("."),
    MakePair(mem, map,
    MakePair(mem, key, nil)));
}

static Val ParseSimple(Val children, Mem *mem)
{
  return Head(mem, children);
}

static Val ParseSymbol(Val children, Mem *mem)
{
  Val name = ListAt(mem, children, 1);
  return
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, name, nil));
}

static Val ParsePrimitiveCall(Val children, Mem *mem)
{
  Val name = Second(mem, children);
  Val args = Third(mem, children);

  return
    MakePair(mem, SymbolFor("@"),
    MakePair(mem, name, args));
}

static Val ParseDoBlock(Val children, Mem *mem)
{
  Val stmts = ListAt(mem, children, 1);
  if (ListLength(mem, stmts) == 1) {
    return Head(mem, stmts);
  } else {
    return MakePair(mem, SymbolFor("do"), ListAt(mem, children, 1));
  }
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
    if (IsInfix(Head(mem, op))) {
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

  if (IsTagged(mem, update, "|")) {
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
    MakePair(mem, First(mem, children), nil));
  Val value = Third(mem, children);

  return
    MakePair(mem, key,
    MakePair(mem, value, nil));
}

static Val ParseUpdate(Val children, Mem *mem)
{
  return ListAt(mem, children, 2);
}

static Val ParseString(Val children, Mem *mem)
{
  Val string = Head(mem, children);
  Val symbol = MakeSymbolFrom(mem, (char*)BinaryData(mem, string), BinaryLength(mem, string));
  return
    MakePair(mem, MakeSymbol(mem, "\""),
    MakePair(mem,
      MakePair(mem, SymbolFor(":"),
      MakePair(mem, symbol, nil)), nil));
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
  case ParseSymPrimCall:    return ParsePrimitiveCall(children, mem);
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
  case ParseSymMap:         return ParseCollection(children, mem);
  case ParseSymEntries:     return ParseSequence(children, mem);
  case ParseSymEntry:       return ParseEntry(children, mem);
  case ParseSymString:      return ParseString(children, mem);
  case ParseSymOptComma:    return nil;
  default:                  return children;
  }
}
