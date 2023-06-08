#include "ast.h"
#include "parse_table.h"

static Val ParseProgram(Val children, Mem *mem);
static Val ParseStmts(Val children, Mem *mem);
static Val ParseStmt(Val children, Mem *mem);
static Val ParseStmtEnd(Val children, Mem *mem);
static Val ParseLetStmt(Val children, Mem *mem);
static Val ParseAssigns(Val children, Mem *mem);
static Val ParseAssign(Val children, Mem *mem);
static Val ParseDefStmt(Val children, Mem *mem);
static Val ParseIdList(Val children, Mem *mem);
static Val ParseCall(Val children, Mem *mem);
static Val ParseArg(Val children, Mem *mem);
static Val ParseLambda(Val children, Mem *mem);
static Val ParseLogic(Val children, Mem *mem);
static Val ParseEquals(Val children, Mem *mem);
static Val ParseCompare(Val children, Mem *mem);
static Val ParseMember(Val children, Mem *mem);
static Val ParseSum(Val children, Mem *mem);
static Val ParseProduct(Val children, Mem *mem);
static Val ParseUnary(Val children, Mem *mem);
static Val ParsePrimary(Val children, Mem *mem);
static Val ParseLiteral(Val children, Mem *mem);
static Val ParseSymbol(Val children, Mem *mem);
static Val ParseAccess(Val children, Mem *mem);
static Val ParseGroup(Val children, Mem *mem);
static Val ParseBlock(Val children, Mem *mem);
static Val ParseDoBlock(Val children, Mem *mem);
static Val ParseIfBlock(Val children, Mem *mem);
static Val ParseDoElse(Val children, Mem *mem);
static Val ParseCondBlock(Val children, Mem *mem);
static Val ParseClauses(Val children, Mem *mem);
static Val ParseClause(Val children, Mem *mem);
static Val ParseObject(Val children, Mem *mem);
static Val ParseList(Val children, Mem *mem);
static Val ParseTuple(Val children, Mem *mem);
static Val ParseMap(Val children, Mem *mem);
static Val ParseItems(Val children, Mem *mem);
static Val ParseItem(Val children, Mem *mem);
static Val ParseEntries(Val children, Mem *mem);
static Val ParseEntry(Val children, Mem *mem);
static Val ParseOptComma(Val children, Mem *mem);

static Val RemoveNils(Val children, Mem *mem);
static Val ParseInfix(Val children, Mem *mem);
static Val Accumulate(Val children, Mem *mem);
static Val TrimEnd(Val children, Mem *mem);
static Val WrapStmts(Val children, Mem *mem);

Val ParseNode(u32 sym, Val children, Mem *mem)
{
  if (IsNil(children)) return nil;

  switch (sym) {
  case ParseSymProgram:   return ParseProgram(children, mem);
  case ParseSymStmts:     return ParseStmts(children, mem);
  case ParseSymStmt:      return ParseStmt(children, mem);
  case ParseSymStmtEnd:   return ParseStmtEnd(children, mem);
  case ParseSymLetStmt:   return ParseLetStmt(children, mem);
  case ParseSymAssigns:   return ParseAssigns(children, mem);
  case ParseSymAssign:    return ParseAssign(children, mem);
  case ParseSymDefStmt:   return ParseDefStmt(children, mem);
  case ParseSymIdList:    return ParseIdList(children, mem);
  case ParseSymCall:      return ParseCall(children, mem);
  case ParseSymArg:       return ParseArg(children, mem);
  case ParseSymLambda:    return ParseLambda(children, mem);
  case ParseSymLogic:     return ParseLogic(children, mem);
  case ParseSymEquals:    return ParseEquals(children, mem);
  case ParseSymCompare:   return ParseCompare(children, mem);
  case ParseSymMember:    return ParseMember(children, mem);
  case ParseSymSum:       return ParseSum(children, mem);
  case ParseSymProduct:   return ParseProduct(children, mem);
  case ParseSymUnary:     return ParseUnary(children, mem);
  case ParseSymPrimary:   return ParsePrimary(children, mem);
  case ParseSymLiteral:   return ParseLiteral(children, mem);
  case ParseSymSymbol:    return ParseSymbol(children, mem);
  case ParseSymAccess:    return ParseAccess(children, mem);
  case ParseSymGroup:     return ParseGroup(children, mem);
  case ParseSymBlock:     return ParseBlock(children, mem);
  case ParseSymDoBlock:   return ParseDoBlock(children, mem);
  case ParseSymIfBlock:   return ParseIfBlock(children, mem);
  case ParseSymDoElse:    return ParseDoElse(children, mem);
  case ParseSymCondBlock: return ParseCondBlock(children, mem);
  case ParseSymClauses:   return ParseClauses(children, mem);
  case ParseSymClause:    return ParseClause(children, mem);
  case ParseSymObject:    return ParseObject(children, mem);
  case ParseSymList:      return ParseList(children, mem);
  case ParseSymTuple:     return ParseTuple(children, mem);
  case ParseSymMap:       return ParseMap(children, mem);
  case ParseSymItems:     return ParseItems(children, mem);
  case ParseSymItem:      return ParseItem(children, mem);
  case ParseSymEntries:   return ParseEntries(children, mem);
  case ParseSymEntry:     return ParseEntry(children, mem);
  case ParseSymOptComma:  return ParseOptComma(children, mem);
  default:                return children;
  }
}

static Val ParseProgram(Val children, Mem *mem)
{
  return WrapStmts(First(mem, children), mem);
}

static Val ParseStmts(Val children, Mem *mem)
{
  return RemoveNils(Accumulate(children, mem), mem);
}

static Val ParseStmt(Val children, Mem *mem)
{
  Val stmt = First(mem, children);
  if (ListLength(mem, stmt) == 1) {
    return First(mem, stmt);
  } else {
    return stmt;
  }
}

static Val ParseStmtEnd(Val children, Mem *mem)
{
  return nil;
}

static Val ParseLetStmt(Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, "let"), Second(mem, children));
}

static Val ParseAssigns(Val children, Mem *mem)
{
  if (ListLength(mem, children) > 1) {
    children = MakePair(mem, Head(mem, children), ListFrom(mem, children, 2));
  }
  return Accumulate(children, mem);
}

static Val ParseAssign(Val children, Mem *mem)
{
  Val id = First(mem, children);
  Val val = Third(mem, children);
  if (ListLength(mem, val) == 1) {
    val = First(mem, val);
  }

  return
    MakePair(mem, id,
    MakePair(mem, val, nil));
}

static Val ParseDefStmt(Val children, Mem *mem)
{
  Val ids = Third(mem, children);
  Val body = Fifth(mem, children);
  Val var = Head(mem, ids);
  Val params = Tail(mem, ids);

  Val lambda =
    MakePair(mem, MakeSymbol(mem, "->"),
    MakePair(mem, params,
    MakePair(mem, body, nil)));

  return
    MakePair(mem, MakeSymbol(mem, "let"),
    MakePair(mem,
      MakePair(mem, var,
      MakePair(mem, lambda, nil)), nil));
}

static Val ParseIdList(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 1) {
    return children;
  } else {
    return ListAppend(mem, First(mem, children), Second(mem, children));
  }
}

static Val ParseCall(Val children, Mem *mem)
{
  return Accumulate(children, mem);
}

static Val ParseArg(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseLambda(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 4) {
    return
      MakePair(mem, MakeSymbol(mem, "->"),
      MakePair(mem, nil,
      MakePair(mem, Fourth(mem, children), nil)));
  }
  return ParseInfix(children, mem);
}

static Val ParseLogic(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseEquals(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseCompare(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseMember(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseSum(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseProduct(Val children, Mem *mem)
{
  return ParseInfix(children, mem);
}

static Val ParseUnary(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 1) {
    return First(mem, children);
  }
  return children;
}

static Val ParsePrimary(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseLiteral(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseSymbol(Val children, Mem *mem)
{
  return children;
}

static Val ParseAccess(Val children, Mem *mem)
{
  return
    MakePair(mem, Second(mem, children),
    MakePair(mem, First(mem, children),
    MakePair(mem, Third(mem, children), nil)));
}

static Val ParseGroup(Val children, Mem *mem)
{
  return Second(mem, children);
}

static Val ParseBlock(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseDoBlock(Val children, Mem *mem)
{
  return WrapStmts(Second(mem, children), mem);
}

static Val ParseIfBlock(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 3) {
    Val do_else = Third(mem, children);
    return
      MakePair(mem, MakeSymbol(mem, "if"),
      MakePair(mem, Second(mem, children),
      MakePair(mem, First(mem, do_else),
      MakePair(mem, Second(mem, do_else), nil))));
  } else {
    return children;
  }
}

static Val ParseDoElse(Val children, Mem *mem)
{
  Val true_stmts = Second(mem, children);
  Val false_stmts = Fourth(mem, children);

  return
    MakePair(mem, WrapStmts(true_stmts, mem),
    MakePair(mem, WrapStmts(false_stmts, mem), nil));
}

static Val ParseCondBlock(Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, "cond"), Third(mem, children));
}

static Val ParseClauses(Val children, Mem *mem)
{
  return RemoveNils(Accumulate(children, mem), mem);
}

static Val ParseClause(Val children, Mem *mem)
{
  Val predicate = First(mem, children);
  Val consequent = Third(mem, children);
  return
    MakePair(mem, predicate,
    MakePair(mem, consequent, nil));
}

static Val ParseObject(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseList(Val children, Mem *mem)
{
  return
    MakePair(mem, MakeSymbol(mem, ":"),
    MakePair(mem, Second(mem, children), nil));
}

static Val ParseTuple(Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, "#["), Second(mem, children));
}

static Val ParseMap(Val children, Mem *mem)
{
  return MakePair(mem, MakeSymbol(mem, "{"), Second(mem, children));
}

static Val ParseItems(Val children, Mem *mem)
{
  return Accumulate(children, mem);
}

static Val ParseItem(Val children, Mem *mem)
{
  return First(mem, children);
}

static Val ParseEntries(Val children, Mem *mem)
{
  return Accumulate(children, mem);
}

static Val ParseEntry(Val children, Mem *mem)
{
  Val key =
    MakePair(mem, MakeSymbol(mem, ":"),
    MakePair(mem, First(mem, children), nil));

  return
    MakePair(mem, key,
    MakePair(mem, Third(mem, children), nil));
}

static Val ParseOptComma(Val children, Mem *mem)
{
  return nil;
}

static Val RemoveNils(Val children, Mem *mem)
{
  Val stmts = nil;
  while (!IsNil(children)) {
    Val stmt = Head(mem, children);
    if (!IsNil(stmt)) {
      stmts = MakePair(mem, stmt, stmts);
    }
    children = Tail(mem, children);
  }
  return ReverseOnto(mem, stmts, nil);
}

static Val ParseInfix(Val children, Mem *mem)
{
  if (ListLength(mem, children) == 1) {
    return First(mem, children);
  } else {
    return
      MakePair(mem, Second(mem, children),
      MakePair(mem, First(mem, children),
      MakePair(mem, Third(mem, children), nil)));
  }
}

static Val Accumulate(Val children, Mem *mem)
{
  if (IsNil(children)) return nil;
  if (ListLength(mem, children) == 1) return children;
  return ListAppend(mem, First(mem, children), Second(mem, children));
}

static Val TrimEnd(Val children, Mem *mem)
{
  if (IsNil(children)) return nil;

  Val items = nil;
  for (u32 i = 0; i < ListLength(mem, children)-1; i++) {
    items = MakePair(mem, Head(mem, children), items);
    children = Tail(mem, children);
  }
  return ReverseOnto(mem, items, nil);
}

static Val WrapStmts(Val stmts, Mem *mem)
{
  if (IsNil(stmts)) return nil;
  if (ListLength(mem, stmts) == 1) return First(mem, stmts);
  return MakePair(mem, MakeSymbol(mem, "do"), stmts);
}
