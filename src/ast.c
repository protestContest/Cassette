#include "ast.h"
#include "parse.h"
#include "parse_syms.h"
#include "print.h"

static bool IsInfix(Val sym);
static Val ParseProgram(Val children, Mem *mem);
static Val ParseSequence(Val children, Mem *mem);
static Val ParseStmt(Val children, Mem *mem);
static Val ParseLetStmt(Val children, Mem *mem);
static Val ParseAssigns(Val children, Mem *mem);
static Val ParseAssign(Val children, Mem *mem);
static Val ParseDefStmt(Val children, Mem *mem);
static Val ParseImportStmt(Val children, Mem *mem);
static Val ParseInfix(Val children, Mem *mem);
static Val ParseMember(Val children, Mem *mem);
static Val ParseLambda(Val children, Mem *mem);
static Val ParseAccess(Val children, Mem *mem);
static Val ParseSimple(Val children, Mem *mem);
static Val ParseSymbol(Val children, Mem *mem);
static Val ParsePrimitiveCall(Val children, Mem *mem);
static Val ParseDoBlock(Val children, Mem *mem);
static Val ParseIfBlock(Val children, Mem *mem);
static Val CondToIf(Val exp, Mem *mem);
static Val ParseCondBlock(Val children, Mem *mem);
static Val ParseClause(Val children, Mem *mem);
static Val ParseGroup(Val children, Mem *mem);
static Val ParseCollection(Val children, Mem *mem);
static Val ParseList(Val children, Mem *mem);
static Val ParseMap(Val children, Mem *mem);
static Val ParseEntry(Val children, Mem *mem);
static Val ParseEntries(Val children, Mem *mem);
static Val ParseUnary(Val children, Mem *mem);

Val ParseNode(u32 sym, Val children, Mem *mem)
{
  if (IsNil(children)) return nil;

  switch (sym) {
  case ParseSymPrimary:     return ParseSimple(children, mem);
  case ParseSymBlock:       return ParseSimple(children, mem);
  case ParseSymDoBlock:     return ParseDoBlock(children, mem);
  case ParseSymIfBlock:     return ParseIfBlock(children, mem);
  case ParseSymCondBlock:   return ParseCondBlock(children, mem);
  case ParseSymProduct:     return ParseInfix(children, mem);
  case ParseSymSum:         return ParseInfix(children, mem);
  case ParseSymMember:      return ParseMember(children, mem);
  case ParseSymCompare:     return ParseInfix(children, mem);
  case ParseSymEquals:      return ParseInfix(children, mem);
  case ParseSymLogic:       return ParseInfix(children, mem);
  case ParseSymLambda:      return ParseLambda(children, mem);
  case ParseSymArg:         return ParseSimple(children, mem);
  case ParseSymCall:        return ParseSequence(children, mem);
  case ParseSymStmt:        return ParseStmt(children, mem);
  case ParseSymStmts:       return ParseSequence(children, mem);
  case ParseSymProgram:     return ParseProgram(children, mem);
  case ParseSymLetStmt:     return ParseLetStmt(children, mem);
  case ParseSymAssigns:     return ParseAssigns(children, mem);
  case ParseSymAssign:      return ParseAssign(children, mem);
  case ParseSymDefStmt:     return ParseDefStmt(children, mem);
  case ParseSymParams:      return ParseSequence(children, mem);
  case ParseSymImportStmt:  return ParseImportStmt(children, mem);
  case ParseSymMlCall:      return ParseSequence(children, mem);
  case ParseSymPrimCall:    return ParsePrimitiveCall(children, mem);
  case ParseSymClauses:     return ParseSequence(children, mem);
  case ParseSymClause:      return ParseClause(children, mem);
  case ParseSymAccess:      return ParseAccess(children, mem);
  case ParseSymLiteral:     return ParseSimple(children, mem);
  case ParseSymSymbol:      return ParseSymbol(children, mem);
  case ParseSymGroup:       return ParseGroup(children, mem);
  case ParseSymList:        return ParseList(children, mem);
  case ParseSymItems:       return ParseSequence(children, mem);
  case ParseSymTuple:       return ParseCollection(children, mem);
  case ParseSymMap:         return ParseMap(children, mem);
  case ParseSymEntries:     return ParseEntries(children, mem);
  case ParseSymEntry:       return ParseEntry(children, mem);
  case ParseSymUnary:       return ParseUnary(children, mem);
  case ParseSymOptComma:    return nil;
  default:                  return children;
  }
}

/* [node] -> node */
static Val ParseSimple(Val children, Mem *mem)
{
  return Head(mem, children);
}

/*
Infix nodes may just be a pass-through for precedence. If not, we rearrange the
expression as a prefix-expression.
[node] -> node
[node op node] -> [op node node]
*/
static Val ParseInfix(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 1) {
    return Head(mem, children);
  } else {
    Val op = Second(mem, children);
    return
      MakePair(mem, op,
      MakePair(mem, First(mem, children),
      MakePair(mem, Third(mem, children), nil)));
  }
}

static Val ParseMember(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 1) {
    return Head(mem, children);
  } else {
    Val op = Second(mem, children);
    return
      MakePair(mem, SymbolFor("@"),
      MakePair(mem, op,
      MakePair(mem, First(mem, children),
      MakePair(mem, Third(mem, children), nil))));
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

static Val ParseEntries(Val children, Mem *mem)
{
  if (IsNil(Tail(mem, children))) {
    Val key = First(mem, First(mem, children));
    Val val = Second(mem, First(mem, children));
    return
      MakePair(mem, MakePair(mem, key, nil),
      MakePair(mem, MakePair(mem, val, nil), nil));
  } else {
    Val keys = First(mem, First(mem, children));
    Val vals = Second(mem, First(mem, children));
    Val new_key = First(mem, Second(mem, children));
    Val new_val = Second(mem, Second(mem, children));
    return
      MakePair(mem, MakePair(mem, new_key, keys),
      MakePair(mem, MakePair(mem, new_val, vals), nil));
  }
}

static bool IsInfix(Val sym)
{
  char *infixes[] = {"+", "-", "*", "/", "<", "<=", ">", ">=", "|", "==", "!=", "and", "or", "in", "->"};
  for (u32 i = 0; i < ArrayCount(infixes); i++) {
    if (Eq(sym, SymbolFor(infixes[i]))) return true;
  }
  return false;
}

/*
A program is parsed into an equivalent do-block
[ [stmt] ] -> [stmt]
[ [stmt, stmt, ...] ] -> [:do [stmt, stmt, ...]]
*/
static Val ParseProgram(Val children, Mem *mem)
{
  Val stmts = Head(mem, children);
  if (ListLength(mem, stmts) == 1) {
    return Head(mem, stmts);
  } else {
    return MakePair(mem, SymbolFor("do"), stmts);
  }
}

/*
A sequence collects each reduced item into a list
[node] -> [node]
[[node, node, ...] node] -> [node, node, node, ...]
*/
static Val ParseSequence(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 1) {
    return ListAppend(mem, First(mem, children), Second(mem, children));
  } else {
    return children;
  }
}

/*
A stmt contains a call. If the call has no arguments, this produces the call
node without invoking it. Otherwise, this produces the call, invoked (as a list
with args).
[ [call] ] -> call
[ [call] ] -> [call]
*/
static Val ParseStmt(Val children, Mem *mem)
{
  Val stmt = Head(mem, children);
  if (ListLength(mem, stmt) == 1) {
    return Head(mem, stmt);
  } else {
    return stmt;
  }
}

/*
A let statement is a call to "let" with each variable and value as successive arguments.
[:let [:x [1] :y [2]]] -> [:let :x [1] :y [2]]
*/
static Val ParseLetStmt(Val children, Mem *mem)
{
  return MakePair(mem, SymbolFor("let"), Second(mem, children));
}

/*
[[:x [1]]] -> [:x [1]]
[[:x [1]] :, [:y [2]]] -> [:x [1] :y [2]]
*/
static Val ParseAssigns(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 1) {
    return ListConcat(mem, First(mem, children), Third(mem, children));
  } else {
    return Head(mem, children);
  }
}

/*
[:x := [[1]]] -> [:x [1]]
[:x := [[[:+ 1 1]]]] -> [:x [[:+ 1 1]]]
*/
static Val ParseAssign(Val children, Mem *mem)
{
  Val value = Third(mem, children);
  if (IsNil(Tail(mem, value))) {
    value = Head(mem, value);
  }

  return
    MakePair(mem, First(mem, children),
    MakePair(mem, value, nil));
}

static Val ParseDefStmt(Val children, Mem *mem)
{
  Val params = Third(mem, children);
  Val var = Head(mem, params);
  params = Tail(mem, params);
  Val body = Fifth(mem, children);

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
    Val filename = Second(mem, children);
    Val var = Fourth(mem, children);
    return
      MakePair(mem, SymbolFor("import"),
      MakePair(mem, filename,
      MakePair(mem, var, nil)));
  } else {
    return MakePair(mem, SymbolFor("import"), Tail(mem, children));
  }
}

static Val ParseLambda(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 3) {
    return
      MakePair(mem, SymbolFor("->"),
      MakePair(mem, nil,
      MakePair(mem, Fourth(mem, children), nil)));
  }
  return ParseInfix(children, mem);
}

static Val ParseAccess(Val children, Mem *mem)
{
  Val map = First(mem, children);
  Val key =
    MakePair(mem, SymbolFor(":"),
    MakePair(mem, Third(mem, children), nil));

  return
    MakePair(mem, SymbolFor("@"),
    MakePair(mem, MakeSymbol(mem, "access"),
    MakePair(mem, map,
    MakePair(mem, key, nil))));
}

static Val ParseSymbol(Val children, Mem *mem)
{
  Val name = Second(mem, children);
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

/*
If a do block contains a single statement, just produce that statement alone
[:do [1] :end] -> 1
[:do [1 2] :end] -> [:do 1 2]
*/
static Val ParseDoBlock(Val children, Mem *mem)
{
  Val stmts = Second(mem, children);
  if (ListLength(mem, stmts) == 1) {
    return Head(mem, stmts);
  } else {
    return MakePair(mem, SymbolFor("do"), Second(mem, children));
  }
}

/*
An if statement has two forms: with or without an "else" block. If it's missing
the "else" block, the consequent has already been parsed as a "do" block, so we
just append "nil" as the alternative. Otherwise, we have to parse the consequent
and alternative blocks here, as we would for a "do" block.

[:if 1 2] -> [:if 1 2 nil]
[:if 1 :do [2] :else [[:foo 3]] :end] -> [:if 1 2 [:foo 3]]
*/
static Val ParseIfBlock(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 3) {
    return MakePair(mem, SymbolFor("if"), ListAppend(mem, Tail(mem, children), nil));
  }

  Val predicate = Second(mem, children);
  Val consequent = Fourth(mem, children);
  Val alternative = Sixth(mem, children);

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
  Val predicate = First(mem, clause);
  Val consequent = Second(mem, clause);
  return
    MakePair(mem, SymbolFor("if"),
    MakePair(mem, predicate,
    MakePair(mem, consequent,
    MakePair(mem, CondToIf(Tail(mem, exp), mem), nil))));
}

/* Cond blocks are transformed to nested if blocks, and parsed that way */
static Val ParseCondBlock(Val children, Mem *mem)
{
  return CondToIf(ListAt(mem, children, 2), mem);
}

/*
The right hand side of a clause is a call, so it follows the same rules as a call
in a statement: it's only invoked when arguments are present
[1 :-> [2]] -> [1 2]
[1 :-> [foo 2]] -> [1 [foo 2]]
*/
static Val ParseClause(Val children, Mem *mem)
{
  Val predicate = First(mem, children);
  Val consequent = Third(mem, children);
  if (ListLength(mem, consequent) == 1) consequent = Head(mem, consequent);
  return
    MakePair(mem, predicate,
    MakePair(mem, consequent, nil));
}

static Val ParseGroup(Val children, Mem *mem)
{
  Val group = Second(mem, children);

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
  Val symbol = First(mem, children);
  Val items = Second(mem, children);
  Val update = Third(mem, children);

  if (IsTagged(mem, update, "|")) {
    // reverse the list here so it's faster to prepend items at runtime
    return ListAppend(mem, update, ReverseOnto(mem, items, nil));
  } else {
    return MakePair(mem, symbol, items);
  }
}

static Val ParseList(Val children, Mem *mem)
{
  u32 length = ListLength(mem, children);
  if (length == 2) {
    return nil;
  } else if (length == 3) {
    Val value = Second(mem, children);
    return
      MakePair(mem, SymbolFor("["),
      MakePair(mem, value, nil));
  } else if (Eq(Third(mem, children), SymbolFor("|"))) {
    Val head = Second(mem, children);
    Val tail = Fourth(mem, children);
    return
      MakePair(mem, SymbolFor("|"),
      MakePair(mem, head,
      MakePair(mem, tail, nil)));
  } else {
    Val first_item = Second(mem, children);
    Val items = Third(mem, children);
    return
      MakePair(mem, SymbolFor("["),
      MakePair(mem, first_item, items));
  }
  return children;
}

static Val ParseMap(Val children, Mem *mem)
{
  Val items = Second(mem, children);
  Val keys = First(mem, items);
  Val vals = Second(mem, items);

  return
    MakePair(mem, SymbolFor("{"),
    MakePair(mem, keys,
    MakePair(mem, vals, nil)));
}

static Val ParseUnary(Val children, Mem *mem)
{
  if (IsNil(Tail(mem, children))) {
    return Head(mem, children);
  } else {
    return children;
  }
}
