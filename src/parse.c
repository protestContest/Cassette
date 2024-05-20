#include "parse.h"
#include "debug.h"
#include <univ.h>

#define IsIDStart(c)  (IsAlpha(c) || (c) == '_')
#define IsIDChar(c)   (IsIDStart(c) || IsDigit(c) || (c) == '?' || (c) == '!')

typedef struct {
  char *text;
  i32 length;
  i32 pos;
} Parser;
#define Peek(p)       ((p)->text[(p)->pos])
#define AtEnd(p)      ((p)->pos >= (p)->length)
#define Adv(p)        ((p)->pos++)
#define Reset(p, pos) ((p)->pos = (pos), p)
#define MatchNL(p)    (Match("\n", p) || Match("\r", p))

bool Match(char *test, Parser *p)
{
  i32 start = p->pos;
  while (*test && !AtEnd(p)) {
    if (*test != Peek(p)) {
      p->pos = start;
      return false;
    }
    Adv(p);
    test++;
  }
  if (*test) {
    p->pos = start;
    return false;
  }
  return true;
}

bool MatchKeyword(char *test, Parser *p)
{
  i32 start = p->pos;
  if (!Match(test, p)) return false;
  if (!AtEnd(p) && IsIDChar(Peek(p))) {
    p->pos = start;
    return false;
  }
  return true;
}

bool CheckKeyword(char *test, Parser *p)
{
  i32 pos = p->pos;
  bool result = MatchKeyword(test, p);
  p->pos = pos;
  return result;
}

val MakeNode(i32 type, i32 pos, val value)
{
  val node = Tuple(4);
  TupleSet(node, 0, IntVal(type));
  TupleSet(node, 1, IntVal(pos));
  TupleSet(node, 2, value);
  TupleSet(node, 3, 0);
  return node;
}
#define ParseError(msg, p)  MakeError(msg, (p)->pos)

val ParseModule(Parser *p);
val ParseModname(Parser *p);
val ParseExports(Parser *p);
val ParseImports(Parser *p);
val ParseStmts(Parser *p);
val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p);
val ParseDef(Parser *p);
val ParseLet(Parser *p);
val ParseAssign(Parser *p);
val ParseExpr(Parser *p);
val ParseLambda(Parser *p);
val ParseLogic(Parser *p);
val ParseEqual(Parser *p);
val ParsePair(Parser *p);
val ParseJoin(Parser *p);
val ParseSplit(Parser *p);
val ParseCompare(Parser *p);
val ParseShift(Parser *p);
val ParseSum(Parser *p);
val ParseProduct(Parser *p);
val ParseUnary(Parser *p);
val ParseCall(Parser *p);
val ParseAccess(Parser *p);
val ParsePrimary(Parser *p);
val ParseGroup(Parser *p);
val ParseDo(Parser *p);
val ParseIf(Parser *p);
val ParseCond(Parser *p);
val ParseClauses(Parser *p);
val ParseList(Parser *p);
val ParseTuple(Parser *p);
val ParseNum(Parser *p);
val ParseByte(Parser *p);
val ParseHex(Parser *p);
val ParseSymbol(Parser *p);
val ParseString(Parser *p);
val ParseID(Parser *p);
val ParseIDList(Parser *p);
val ParseArgs(Parser *p);
void Spacing(Parser *p);
void VSpacing(Parser *p);

val Parse(char *text, i32 length)
{
  Parser p;
  p.text = text;
  p.length = length;
  p.pos = 0;
  return ParseModule(&p);
}

val ParseModule(Parser *p)
{
  i32 pos, modname, exports, imports, stmts;
  VSpacing(p);
  pos = p->pos;
  if (CheckKeyword("module", p)) {
    modname = ParseModname(p);
    if (IsError(modname)) return modname;
    if (CheckKeyword("exports", p)) {
      exports = ParseExports(p);
      if (IsError(exports)) return exports;
    } else {
      exports = MakeNode(exportNode, pos, 0);
    }
  } else {
    modname = MakeNode(idNode, pos, SymVal(Symbol("*main*")));
    exports = MakeNode(exportNode, pos, 0);
  }
  imports = ParseImports(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  return MakeNode(moduleNode, pos, Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

val ParseModname(Parser *p)
{
  i32 id;
  if (!MatchKeyword("module", p)) return ParseError("Expected \"module\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return id;
}

val ParseExports(Parser *p)
{
  i32 exports, pos;
  if (!MatchKeyword("exports", p)) return ParseError("Expected \"exports\"", p);
  Spacing(p);
  pos = p->pos;
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return MakeNode(exportNode, pos, NodeValue(exports));
}

val ParseImports(Parser *p)
{
  i32 imports = 0, start = p->pos;
  while (MatchKeyword("import", p)) {
    i32 import, id, alias, pos;
    Spacing(p);
    pos = p->pos;
    id = ParseID(p);
    if (IsError(id)) return id;
    if (MatchKeyword("as", p)) {
      Spacing(p);
      alias = ParseID(p);
      if (IsError(alias)) return alias;
    } else {
      alias = id;
    }
    import = MakeNode(listNode, pos, Pair(id, Pair(alias, 0)));
    imports = Pair(import, imports);
    if (!AtEnd(p) && !MatchNL(p)) return ParseError("Expected newline", p);
    VSpacing(p);
  }
  return MakeNode(importNode, start, ReverseList(imports, 0));
}

val ParseStmts(Parser *p)
{
  val stmts = 0, defs = 0, stmt;
  i32 pos = p->pos, numAssigns = 0;
  stmt = ParseStmt(&stmts, &defs, &numAssigns, p);
  if (IsError(stmt)) return stmt;
  while (!AtEnd(p) && IsNewline(Peek(p))) {
    VSpacing(p);
    if (AtEnd(p) || CheckKeyword("end", p) || CheckKeyword("else", p)) break;
    stmt = ParseStmt(&stmts, &defs, &numAssigns, p);
    if (IsError(stmt)) return stmt;
  }
  VSpacing(p);
  stmts = ReverseList(stmts, 0);
  stmts = ReverseList(defs, stmts);
  return MakeNode(doNode, pos, Pair(MakeNode(intNode, pos, IntVal(numAssigns)), stmts));
}

val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p)
{
  val stmt;
  if (CheckKeyword("def", p)) {
    stmt = ParseDef(p);
    if (IsError(stmt)) return stmt;
    *defs = Pair(stmt, *defs);
    (*numAssigns)++;
  } else if (CheckKeyword("let", p)) {
    stmt = ParseLet(p);
    if (IsError(stmt)) return stmt;
    *stmts = Pair(stmt, *stmts);
    (*numAssigns) += NodeCount(stmt);
  } else {
    stmt = ParseExpr(p);
    if (IsError(stmt)) return stmt;
    *stmts = Pair(stmt, *stmts);
  }
  return MakeNode(nilNode, 0, 0);
}

val ParseDef(Parser *p)
{
  i32 id, lambda, params = 0, body, pos = p->pos;
  if (!MatchKeyword("def", p)) return ParseError("Expected \"def\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!Match("(", p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  if (!Match(")", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match(")", p)) return ParseError("Expected \")\"", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  lambda = MakeNode(lambdaNode, pos, Pair(params, Pair(body, 0)));
  return MakeNode(defNode, pos, Pair(id, Pair(lambda, 0)));
}

val ParseLet(Parser *p)
{
  i32 assigns = 0, assign, pos = p->pos;
  if (!MatchKeyword("let", p)) return ParseError("Expected \"let\"", p);
  Spacing(p);
  assign = ParseAssign(p);
  if (IsError(assign)) return assign;
  assigns = Pair(assign, assigns);
  while (Match(",", p)) {
    VSpacing(p);
    assign = ParseAssign(p);
    if (IsError(assign)) return assign;
    assigns = Pair(assign, assigns);
  }
  return MakeNode(letNode, pos, ReverseList(assigns, 0));
}

val ParseAssign(Parser *p)
{
  i32 id, value, pos = p->pos;
  id = ParseID(p);
  if (IsError(id)) return id;
  Spacing(p);
  if (!Match("=", p)) return ParseError("Expected \"=\"", p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsError(value)) return value;
  return MakeNode(listNode, pos, Pair(id, Pair(value, 0)));
}

val ParseExpr(Parser *p)
{
  if (Peek(p) == '\\') return ParseLambda(p);
  return ParseLogic(p);
}

val ParseLambda(Parser *p)
{
  i32 params = 0, body, pos = p->pos;
  if (!Match("\\", p)) return ParseError("Expected \"\\\"", p);
  Spacing(p);
  if (!Match("->", p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!Match("->", p)) return ParseError("Expected \"->\"", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  return MakeNode(lambdaNode, pos, Pair(params, Pair(body, 0)));
}

val ParseLogic(Parser *p)
{
  i32 expr = ParseEqual(p), pos, arg;
  i32 op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = MatchKeyword("and", p) ? andNode : MatchKeyword("or", p) ? orNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseEqual(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = MatchKeyword("and", p) ? andNode : MatchKeyword("or", p) ? orNode : 0;
  }
  return expr;
}

val ParseEqual(Parser *p)
{
  i32 pos, expr;
  expr = ParsePair(p);
  if (IsError(expr)) return expr;
  pos = p->pos;
  if (Match("==", p)) {
    i32 arg;
    VSpacing(p);
    arg = ParsePair(p);
    if (IsError(arg)) return arg;
    return MakeNode(eqNode, pos, Pair(expr, Pair(arg, 0)));
  } else if (Match("!=", p)) {
    i32 arg;
    VSpacing(p);
    arg = ParsePair(p);
    if (IsError(arg)) return arg;
    return MakeNode(notNode, pos, Pair(MakeNode(eqNode, pos, Pair(expr, Pair(arg, 0))), 0));
  }
  return expr;
}

val ParsePair(Parser *p)
{
  i32 expr = ParseJoin(p), arg, pos;
  if (IsError(expr)) return expr;
  pos = p->pos;
  while (Match("|", p)) {
    VSpacing(p);
    arg = ParseJoin(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(pairNode, pos, Pair(arg, Pair(expr, 0)));
    pos = p->pos;
  }
  return expr;
}

val ParseJoin(Parser *p)
{
  i32 expr = ParseSplit(p), arg, pos;
  if (IsError(expr)) return expr;
  pos = p->pos;
  while (Match("<>", p)) {
    VSpacing(p);
    arg = ParseSplit(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(joinNode, pos, Pair(arg, Pair(expr, 0)));
    pos = p->pos;
  }
  return expr;
}

val ParseSplit(Parser *p)
{
  i32 expr = ParseCompare(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match(":", p) ? truncNode : Match("@", p) ? skipNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseCompare(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match(":", p) ? truncNode : Match("@", p) ? skipNode : 0;
  }
  return expr;
}

val ParseCompare(Parser *p)
{
  i32 expr = ParseShift(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("<", p) ? ltNode : Match(">", p) ? gtNode : 0;
  if (Match(">", p)) {
    p->pos = pos;
    op = 0;
  }
  while (op) {
    VSpacing(p);
    arg = ParseShift(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("<", p) ? ltNode : Match(">", p) ? gtNode : 0;
    if (Match(">", p)) {
      p->pos = pos;
      op = 0;
    }
  }
  return expr;
}

val ParseShift(Parser *p)
{
  i32 expr = ParseSum(p), arg, pos, op = 0;
  bool right = false;
  if (IsError(expr)) return expr;
  pos = p->pos;

  if (Match("<<", p)) {
    op = shiftNode;
    right = false;
  } else if (Match(">>", p)) {
    op = shiftNode;
    right = true;
  } else {
    op = 0;
  }

  while (op) {
    VSpacing(p);
    arg = ParseSum(p);
    if (IsError(arg)) return arg;
    if (right) arg = MakeNode(negNode, NodePos(arg), Pair(arg, 0));
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    if (Match("<<", p)) {
      op = shiftNode;
      right = false;
    } else if (Match(">>", p)) {
      op = shiftNode;
      right = true;
    } else {
      op = 0;
    }
  }
  return expr;
}

val ParseSum(Parser *p)
{
  i32 expr = ParseProduct(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("+", p) ? addNode :
       Match("-", p) ? subNode :
       Match("^", p) ? borNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseProduct(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("+", p) ? addNode :
         Match("-", p) ? subNode :
         Match("^", p) ? borNode : 0;
  }
  return expr;
}

val ParseProduct(Parser *p)
{
  i32 expr = ParseUnary(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->pos;
  op = Match("*", p) ? mulNode :
       Match("/", p) ? divNode :
       Match("%", p) ? remNode :
       Match("&", p) ? bandNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseUnary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
    op = Match("*", p) ? mulNode :
         Match("/", p) ? divNode :
         Match("%", p) ? remNode :
         Match("&", p) ? bandNode : 0;
  }
  return expr;
}

val ParseUnary(Parser *p)
{
  i32 pos = p->pos, expr;
  if (Match("-", p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(negNode, pos, Pair(expr, 0));
  } else if (Match("~", p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(compNode, pos, Pair(expr, 0));
  } else if (Match("#", p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(lenNode, pos, Pair(expr, 0));
  } else if (MatchKeyword("not", p)) {
    Spacing(p);
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(notNode, pos, Pair(expr, 0));
  } else {
    return ParseCall(p);
  }
}

val ParseCall(Parser *p)
{
  i32 pos;
  val expr = ParseAccess(p);
  if (IsError(expr)) return expr;
  pos = p->pos;
  while (Match("(", p)) {
    val args;
    VSpacing(p);
    args = 0;
    if (!Match(")", p)) {
      args = ParseArgs(p);
      if (IsError(args)) return args;
      expr = MakeNode(callNode, pos, Pair(expr, ReverseList(NodeValue(args), 0)));
      VSpacing(p);
      if (!Match(")", p)) return ParseError("Expected \")\"", p);
    }
    pos = p->pos;
  }
  Spacing(p);
  return expr;
}

val ParseAccess(Parser *p)
{
  i32 pos, expr = ParsePrimary(p);
  if (IsError(expr)) return expr;
  pos = p->pos;
  while (Match(".", p)) {
    i32 arg = ParsePrimary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(accessNode, pos, Pair(expr, Pair(arg, 0)));
    pos = p->pos;
  }
  return expr;
}

val ParsePrimary(Parser *p)
{
  i32 pos = p->pos;
  if (Peek(p) == '(') {
    return ParseGroup(p);
  } else if (Peek(p) == '[') {
    return ParseList(p);
  } else if (Peek(p) == '{') {
    return ParseTuple(p);
  } else if (CheckKeyword("do", p)) {
    return ParseDo(p);
  } else if (CheckKeyword("if", p)) {
    return ParseIf(p);
  } else if (CheckKeyword("cond", p)) {
    return ParseCond(p);
  } else if (IsDigit(Peek(p))) {
    return ParseNum(p);
  } else if (Peek(p) == ':') {
    return ParseSymbol(p);
  } else if (Peek(p) == '$') {
    return ParseByte(p);
  } else if (Peek(p) == '"') {
    return ParseString(p);
  } else if (MatchKeyword("nil", p)) {
    return MakeNode(nilNode, pos, 0);
  } else if (MatchKeyword("true", p)) {
    return MakeNode(intNode, pos, IntVal(1));
  } else if (MatchKeyword("false", p)) {
    return MakeNode(intNode, pos, IntVal(0));
  } else if (IsIDStart(Peek(p))) {
    return ParseID(p);
  } else {
    return ParseError("Unexpected expression", p);
  }
}

val ParseGroup(Parser *p)
{
  i32 expr;
  if (!Match("(", p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  expr = ParseExpr(p);
  if (IsError(expr)) return expr;
  VSpacing(p);
  if (!Match(")", p)) return ParseError("Expected \")\"", p);
  Spacing(p);
  return expr;
}

#define TrivialDoNode(node)   (RawVal(NodeValue(NodeChild(node, 0))) == 0 && NodeCount(node) == 2)
val ParseDo(Parser *p)
{
  i32 stmts, pos = p->pos;
  if (!MatchKeyword("do", p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!MatchKeyword("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  if (TrivialDoNode(stmts)) return NodeChild(stmts, 1);
  return MakeNode(doNode, pos, NodeValue(stmts));
}

val ParseIf(Parser *p)
{
  i32 pred, cons, alt;
  i32 start = p->pos, pos;
  if (!MatchKeyword("if", p)) return ParseError("Expected \"if\"", p);
  Spacing(p);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  pos = p->pos;
  if (!MatchKeyword("do", p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  cons = ParseStmts(p);
  if (IsError(cons)) return cons;
  if (TrivialDoNode(cons)) cons = NodeChild(cons, 1);
  pos = p->pos;
  if (MatchKeyword("else", p)) {
    VSpacing(p);
    alt = ParseStmts(p);
    if (IsError(alt)) return alt;
    if (TrivialDoNode(alt)) alt = NodeChild(alt, 1);
  } else {
    alt = MakeNode(nilNode, pos, 0);
  }
  if (!MatchKeyword("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return MakeNode(ifNode, start, Pair(pred, Pair(cons, Pair(alt, 0))));
}

val ParseCond(Parser *p)
{
  i32 expr;
  if (!MatchKeyword("cond", p)) return ParseError("Expected \"cond\"", p);
  expr = ParseClauses(p);
  if (IsError(expr)) return expr;
  if (!Match("end", p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return expr;
}

val ParseClauses(Parser *p)
{
  i32 pred, cons, alt, pos = p->pos;
  VSpacing(p);
  if (CheckKeyword("end", p)) return MakeNode(nilNode, p->pos, 0);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  if (!Match("->", p)) return ParseError("Expected \"->\"", p);
  VSpacing(p);
  cons = ParseExpr(p);
  if (IsError(cons)) return cons;
  alt = ParseClauses(p);
  if (IsError(alt)) return alt;
  return MakeNode(ifNode, pos, Pair(pred, Pair(cons, Pair(alt, 0))));
}

val ParseList(Parser *p)
{
  i32 pos = p->pos;
  i32 items = 0;
  if (!Match("[", p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (!Match("]", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (!Match("]", p)) return ParseError("Expected \"]\"", p);
  }
  Spacing(p);
  return MakeNode(listNode, pos, NodeValue(items));
}

val ParseTuple(Parser *p)
{
  i32 pos = p->pos, items = 0;
  if (!Match("{", p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (!Match("}", p)) {
    items = ParseArgs(p);
    if (IsError(items)) return items;
    if (!Match("}", p)) return ParseError("Expected \"}\"", p);
  }
  Spacing(p);
  return MakeNode(tupleNode, pos, ReverseList(NodeValue(items), 0));
}

val ParseNum(Parser *p)
{
  i32 n = 0, pos = p->pos;
  if (Match("0x", p)) return ParseHex(p);
  if (!IsDigit(Peek(p))) return ParseError("Expected number", p);
  while (!AtEnd(p) && IsDigit(Peek(p))) {
    i32 d = Peek(p) - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
    Adv(p);
  }
  Spacing(p);
  return MakeNode(intNode, pos, IntVal(n));
}

val ParseByte(Parser *p)
{
  u8 byte;
  i32 pos = p->pos;
  if (!Match("$", p)) return ParseError("Expected \"$\"", p);
  if (AtEnd(p) || IsSpace(Peek(p)) || !IsPrintable(Peek(p))) return ParseError("Expected character", p);
  byte = Peek(p);
  Adv(p);
  Spacing(p);
  return MakeNode(intNode, pos, IntVal(byte));
}

val ParseHex(Parser *p)
{
  i32 n = 0, pos = p->pos;
  if (!IsHexDigit(Peek(p))) return ParseError("Expected number", p);
  while (!AtEnd(p) && IsHexDigit(Peek(p))) {
    i32 d = IsDigit(Peek(p)) ? Peek(p) - '0' : Peek(p) - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
    Adv(p);
  }
  Spacing(p);
  return MakeNode(intNode, pos, IntVal(n));
}

val ParseSymbol(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!Match(":", p)) return ParseError("Expected \":\"", p);
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos + 1, p->pos - pos - 1);
  Spacing(p);
  return MakeNode(symNode, pos, SymVal(sym));
}

val ParseString(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!Match("\"", p)) return ParseError("Expected quote", p);
  while (!AtEnd(p) && Peek(p) != '"') {
    if (Peek(p) == '\\') Adv(p);
    Adv(p);
  }
  sym = SymbolFrom(p->text + pos + 1, p->pos - pos - 1);
  if (!Match("\"", p)) return ParseError("Expected quote", p);
  Spacing(p);
  return MakeNode(strNode, pos, SymVal(sym));
}

bool IsKeyword(u32 sym)
{
  if (sym == Symbol("module")) return true;
  if (sym == Symbol("exports")) return true;
  if (sym == Symbol("import")) return true;
  if (sym == Symbol("def")) return true;
  if (sym == Symbol("let")) return true;
  if (sym == Symbol("and")) return true;
  if (sym == Symbol("or")) return true;
  if (sym == Symbol("do")) return true;
  if (sym == Symbol("cond")) return true;
  if (sym == Symbol("if")) return true;
  if (sym == Symbol("else")) return true;
  if (sym == Symbol("end")) return true;
  if (sym == Symbol("not")) return true;
  return false;
}

val ParseID(Parser *p)
{
  i32 sym, pos = p->pos;
  if (!IsIDStart(Peek(p))) return ParseError("Expected identifier", p);
  while (!AtEnd(p) && IsIDChar(Peek(p))) Adv(p);
  sym = SymbolFrom(p->text + pos, p->pos - pos);
  if (IsKeyword(sym)) return ParseError("Reserved word", Reset(p, pos));
  Spacing(p);
  return MakeNode(idNode, pos, SymVal(sym));
}

val ParseIDList(Parser *p)
{
  i32 items = 0;
  i32 pos = p->pos;
  i32 id = ParseID(p);
  if (IsError(id)) return id;
  items = Pair(id, items);
  while (Match(",", p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    items = Pair(id, items);
  }
  return MakeNode(listNode, pos, ReverseList(items, 0));
}

val ParseArgs(Parser *p)
{
  i32 items = 0;
  i32 item = ParseExpr(p);
  i32 pos = p->pos;
  if (IsError(item)) return item;
  items = Pair(item, items);
  while (Match(",", p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsError(item)) return item;
    items = Pair(item, items);
  }
  return MakeNode(listNode, pos, items);
}

void Spacing(Parser *p)
{
  while (!AtEnd(p) && IsSpace(Peek(p))) Adv(p);
  if (!AtEnd(p) && Peek(p) == ';') {
    while (!AtEnd(p) && !IsNewline(Peek(p))) Adv(p);
    Adv(p);
    Spacing(p);
  }
}

void VSpacing(Parser *p)
{
  Spacing(p);
  if (!AtEnd(p) && IsNewline(Peek(p))) {
    Adv(p);
    VSpacing(p);
  }
}

char *NodeName(i32 type)
{
  switch (type) {
  case errNode:     return "error";
  case nilNode:     return "nil";
  case idNode:      return "id";
  case intNode:     return "num";
  case symNode:     return "sym";
  case strNode:     return "str";
  case pairNode:    return "pair";
  case joinNode:    return "join";
  case truncNode:   return "trunc";
  case skipNode:    return "skip";
  case listNode:    return "list";
  case tupleNode:   return "tuple";
  case doNode:      return "do";
  case ifNode:      return "if";
  case andNode:     return "and";
  case orNode:      return "or";
  case eqNode:      return "eq";
  case ltNode:      return "lt";
  case gtNode:      return "gt";
  case shiftNode:   return "shift";
  case addNode:     return "add";
  case subNode:     return "sub";
  case borNode:     return "bitOr";
  case mulNode:     return "mul";
  case divNode:     return "div";
  case remNode:     return "rem";
  case bandNode:    return "bitAnd";
  case negNode:     return "neg";
  case notNode:     return "not";
  case compNode:    return "comp";
  case lenNode:     return "len";
  case callNode:    return "call";
  case accessNode:  return "access";
  case lambdaNode:  return "lambda";
  case letNode:     return "let";
  case defNode:     return "def";
  case importNode:  return "import";
  case exportNode:  return "export";
  case moduleNode:  return "module";
  default:          return "?";
  }
}

void PrintNode(val node, i32 level, u32 lines)
{
  i32 type = NodeType(node);
  i32 i, subnodes;

  if (!type) {
    debug("nil:%d\n", NodePos(node));
    return;
  }

  debug("%s:%d", NodeName(type), NodePos(node));
  switch (type) {
  case idNode:
    debug(" %s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case strNode:
    debug(" \"%s\"\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case symNode:
    debug(" :%s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case intNode:
    debug(" %d\n", RawVal(NodeValue(node)));
    break;
  default:
    subnodes = NodeValue(node);
    debug("\n");
    lines |= Bit(level);
    while (subnodes && IsPair(subnodes)) {
      for (i = 0; i < level; i++) {
        if (lines & Bit(i)) {
          debug("│ ");
        } else {
          debug("  ");
        }
      }
      if (Tail(subnodes)) {
        debug("├ ");
      } else {
        lines &= ~Bit(level);
        debug("└ ");
      }
      PrintNode(Head(subnodes), level+1, lines);
      subnodes = Tail(subnodes);
    }
  }
}

void PrintAST(val node)
{
  PrintNode(node, 0, 0);
}

void PrintError(val node, char *source)
{
  i32 pos = NodePos(node);
  i32 col = 0, i;
  i32 line = 0;
  if (!node) return;
  fprintf(stderr, "%sError: %s\n", ANSIRed, ErrorMsg(node));
  if (source) {
    char *start = source + pos;
    char *end = source + pos;
    while (start > source && !IsNewline(start[-1])) {
      col++;
      start--;
    }
    while (*end && !IsNewline(*end)) end++;
    for (i = 0; i < pos; i++) {
      if (IsNewline(source[i])) line++;
    }
    fprintf(stderr, "%3d| %*.*s\n", line+1, (i32)(end-start), (i32)(end-start), start);
    fprintf(stderr, "     ");
    for (i = 0; i < col; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n");
  }
  fprintf(stderr, "%s", ANSINormal);
}
