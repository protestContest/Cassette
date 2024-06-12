#include "parse.h"
#include "lex.h"
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/math.h>

typedef struct {
  char *text;
  Token token;
} Parser;
#define Adv(p)              (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
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
#define ParseError(msg, p)  MakeError(msg, (p)->token.pos)

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
val ParseItems(Parser *p);
void Spacing(Parser *p);
void VSpacing(Parser *p);

val Parse(char *text)
{
  Parser p;
  p.text = text;
  p.token.pos = 0;
  p.token.length = 0;
  Adv(&p);
  return ParseModule(&p);
}

val ParseModule(Parser *p)
{
  i32 pos = p->token.pos, modname, exports, imports, stmts;
  VSpacing(p);
  if (p->token.type == moduleToken) {
    modname = ParseModname(p);
    if (IsError(modname)) return modname;
    if (p->token.type == exportsToken) {
      exports = ParseExports(p);
      if (IsError(exports)) return exports;
    } else {
      exports = MakeNode(exportNode, p->token.pos, 0);
    }
  } else {
    modname = MakeNode(idNode, p->token.pos, SymVal(Symbol("*main*")));
    exports = MakeNode(exportNode, p->token.pos, 0);
  }
  imports = ParseImports(p);
  if IsError(imports) return imports;
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  return MakeNode(moduleNode, pos, Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

val ParseModname(Parser *p)
{
  i32 id;
  if (!MatchToken(moduleToken, p)) return ParseError("Expected \"module\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return id;
}

val ParseExports(Parser *p)
{
  i32 exports, pos = p->token.pos;
  if (!MatchToken(exportsToken, p)) return ParseError("Expected \"exports\"", p);
  Spacing(p);
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) return ParseError("Expected newline", p);
  VSpacing(p);
  return MakeNode(exportNode, pos, NodeValue(exports));
}

val ParseImports(Parser *p)
{
  i32 imports = 0, start = p->token.pos;
  while (MatchToken(importToken, p)) {
    i32 import, id, alias, pos;
    Spacing(p);
    pos = p->token.pos;
    id = ParseID(p);
    if (IsError(id)) return id;
    if (MatchToken(asToken, p)) {
      Spacing(p);
      alias = ParseID(p);
      if (IsError(alias)) return alias;
    } else {
      alias = id;
    }
    import = MakeNode(listNode, pos, Pair(id, Pair(alias, 0)));
    imports = Pair(import, imports);
    if (!AtEnd(p) && !MatchToken(newlineToken, p)) return ParseError("Expected newline", p);
    VSpacing(p);
  }
  return MakeNode(importNode, start, ReverseList(imports, 0));
}

val ParseStmts(Parser *p)
{
  val stmts = 0, defs = 0, stmt;
  i32 pos = p->token.pos, numAssigns = 0;
  stmt = ParseStmt(&stmts, &defs, &numAssigns, p);
  if (IsError(stmt)) return stmt;
  while (!AtEnd(p) && CheckToken(newlineToken, p)) {
    VSpacing(p);
    if (AtEnd(p) || CheckToken(endToken, p) || CheckToken(elseToken, p)) break;
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
  if (CheckToken(defToken, p)) {
    stmt = ParseDef(p);
    if (IsError(stmt)) return stmt;
    *defs = Pair(stmt, *defs);
    (*numAssigns)++;
  } else if (CheckToken(letToken, p)) {
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
  i32 id, lambda, params = 0, body, pos = p->token.pos;
  if (!MatchToken(defToken, p)) return ParseError("Expected \"def\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  }
  VSpacing(p);
  body = ParseExpr(p);
  if (IsError(body)) return body;
  lambda = MakeNode(lambdaNode, pos, Pair(params, Pair(body, 0)));
  return MakeNode(defNode, pos, Pair(id, Pair(lambda, 0)));
}

val ParseLet(Parser *p)
{
  i32 assigns = 0, assign, pos = p->token.pos;
  if (!MatchToken(letToken, p)) return ParseError("Expected \"let\"", p);
  Spacing(p);
  assign = ParseAssign(p);
  if (IsError(assign)) return assign;
  assigns = Pair(assign, assigns);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    assign = ParseAssign(p);
    if (IsError(assign)) return assign;
    assigns = Pair(assign, assigns);
  }
  return MakeNode(letNode, pos, ReverseList(assigns, 0));
}

val ParseAssign(Parser *p)
{
  i32 id, value, pos = p->token.pos;
  id = ParseID(p);
  if (IsError(id)) return id;
  Spacing(p);
  if (!MatchToken(eqToken, p)) return ParseError("Expected \"=\"", p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsError(value)) return value;
  return MakeNode(listNode, pos, Pair(id, Pair(value, 0)));
}

val ParseExpr(Parser *p)
{
  if (CheckToken(bslashToken, p)) return ParseLambda(p);
  return ParseLogic(p);
}

val ParseLambda(Parser *p)
{
  i32 params = 0, body, pos = p->token.pos;
  if (!MatchToken(bslashToken, p)) return ParseError("Expected \"\\\"", p);
  Spacing(p);
  if (!MatchToken(arrowToken, p)) {
    params = ParseIDList(p);
    if (IsError(params)) return params;
    if (!MatchToken(arrowToken, p)) return ParseError("Expected \"->\"", p);
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
  pos = p->token.pos;
  op = MatchToken(andToken, p) ? andNode : MatchToken(orToken, p) ? orNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseEqual(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
    op = MatchToken(andToken, p) ? andNode : MatchToken(orToken, p) ? orNode : 0;
  }
  return expr;
}

val ParseEqual(Parser *p)
{
  i32 pos, expr;
  expr = ParsePair(p);
  if (IsError(expr)) return expr;
  pos = p->token.pos;
  if (MatchToken(eqeqToken, p)) {
    i32 arg;
    VSpacing(p);
    arg = ParsePair(p);
    if (IsError(arg)) return arg;
    return MakeNode(eqNode, pos, Pair(expr, Pair(arg, 0)));
  } else if (MatchToken(bangeqToken, p)) {
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
  pos = p->token.pos;
  while (MatchToken(barToken, p)) {
    VSpacing(p);
    arg = ParseJoin(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(pairNode, pos, Pair(arg, Pair(expr, 0)));
    pos = p->token.pos;
  }
  return expr;
}

val ParseJoin(Parser *p)
{
  i32 expr = ParseCompare(p), arg, pos;
  if (IsError(expr)) return expr;
  pos = p->token.pos;
  while (MatchToken(ltgtToken, p)) {
    VSpacing(p);
    arg = ParseCompare(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(joinNode, pos, Pair(arg, Pair(expr, 0)));
    pos = p->token.pos;
  }
  return expr;
}

val ParseCompare(Parser *p)
{
  i32 expr = ParseShift(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->token.pos;
  op = MatchToken(ltToken, p) ? ltNode : MatchToken(gtToken, p) ? gtNode : 0;
  if (MatchToken(gtToken, p)) {
    p->token.pos = pos;
    op = 0;
  }
  while (op) {
    VSpacing(p);
    arg = ParseShift(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
    op = MatchToken(ltToken, p) ? ltNode : MatchToken(gtToken, p) ? gtNode : 0;
    if (MatchToken(gtToken, p)) {
      p->token.pos = pos;
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
  pos = p->token.pos;

  if (MatchToken(ltltToken, p)) {
    op = shiftNode;
    right = false;
  } else if (MatchToken(gtgtToken, p)) {
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
    pos = p->token.pos;
    if (MatchToken(ltltToken, p)) {
      op = shiftNode;
      right = false;
    } else if (MatchToken(gtgtToken, p)) {
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
  pos = p->token.pos;
  op = MatchToken(plusToken, p) ? addNode :
       MatchToken(minusToken, p) ? subNode :
       MatchToken(caretToken, p) ? borNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseProduct(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
    op = MatchToken(plusToken, p) ? addNode :
         MatchToken(minusToken, p) ? subNode :
         MatchToken(caretToken, p) ? borNode : 0;
  }
  return expr;
}

val ParseProduct(Parser *p)
{
  i32 expr = ParseUnary(p), arg, pos, op;
  if (IsError(expr)) return expr;
  pos = p->token.pos;
  op = MatchToken(starToken, p) ? mulNode :
       MatchToken(slashToken, p) ? divNode :
       MatchToken(percentToken, p) ? remNode :
       MatchToken(ampToken, p) ? bandNode : 0;
  while (op) {
    VSpacing(p);
    arg = ParseUnary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(op, pos, Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
    op = MatchToken(starToken, p) ? mulNode :
         MatchToken(slashToken, p) ? divNode :
         MatchToken(percentToken, p) ? remNode :
         MatchToken(ampToken, p) ? bandNode : 0;
  }
  return expr;
}

val ParseUnary(Parser *p)
{
  i32 pos = p->token.pos, expr;
  if (MatchToken(minusToken, p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(negNode, pos, Pair(expr, 0));
  } else if (MatchToken(tildeToken, p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(compNode, pos, Pair(expr, 0));
  } else if (MatchToken(hashToken, p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(lenNode, pos, Pair(expr, 0));
  } else if (MatchToken(notToken, p)) {
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

  while (CheckToken(lparenToken, p) || CheckToken(lbracketToken, p)) {
    pos = p->token.pos;
    if (MatchToken(lparenToken, p)) {
      val args = 0;
      VSpacing(p);
      if (!MatchToken(rparenToken, p)) {
        args = ParseItems(p);
        if (IsError(args)) return args;
        VSpacing(p);
        if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
      }
      expr = MakeNode(callNode, pos, Pair(expr, ReverseList(NodeValue(args), 0)));
    } else if (MatchToken(lbracketToken, p)) {
      val start;
      VSpacing(p);
      start = ParseExpr(p);
      if (IsError(start)) return start;
      VSpacing(p);
      if (MatchToken(colonToken, p)) {
        val end;
        if (CheckToken(rbracketToken, p)) {
          end = MakeNode(nilNode, p->token.pos, 0);
        } else {
          end = ParseExpr(p);
        }
        if (IsError(end)) return end;
        expr = MakeNode(sliceNode, pos, Pair(expr, Pair(start, Pair(end, 0))));
        VSpacing(p);
      } else {
        expr = MakeNode(accessNode, pos, Pair(expr, Pair(start, 0)));
      }
      if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
    }
  }
  Spacing(p);
  return expr;
}

val ParseAccess(Parser *p)
{
  i32 pos, expr = ParsePrimary(p);
  if (IsError(expr)) return expr;
  pos = p->token.pos;
  while (MatchToken(dotToken, p)) {
    i32 arg = ParsePrimary(p);
    if (IsError(arg)) return arg;
    expr = MakeNode(accessNode, pos, Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
  }
  return expr;
}

val ParsePrimary(Parser *p)
{
  i32 pos = p->token.pos;
  if (CheckToken(lparenToken, p)) {
    return ParseGroup(p);
  } else if (CheckToken(lbracketToken, p)) {
    return ParseList(p);
  } else if (CheckToken(lbraceToken, p)) {
    return ParseTuple(p);
  } else if (CheckToken(doToken, p)) {
    return ParseDo(p);
  } else if (CheckToken(ifToken, p)) {
    return ParseIf(p);
  } else if (CheckToken(condToken, p)) {
    return ParseCond(p);
  } else if (CheckToken(numToken, p)) {
    return ParseNum(p);
  } else if (CheckToken(hexToken, p)) {
    return ParseHex(p);
  } else if (CheckToken(colonToken, p)) {
    return ParseSymbol(p);
  } else if (CheckToken(byteToken, p)) {
    return ParseByte(p);
  } else if (CheckToken(stringToken, p)) {
    return ParseString(p);
  } else if (MatchToken(nilToken, p)) {
    return MakeNode(nilNode, pos, 0);
  } else if (MatchToken(trueToken, p)) {
    return MakeNode(intNode, pos, IntVal(1));
  } else if (MatchToken(falseToken, p)) {
    return MakeNode(intNode, pos, IntVal(0));
  } else if (CheckToken(idToken, p)) {
    return ParseID(p);
  } else {
    return ParseError("Unexpected expression", p);
  }
}

val ParseGroup(Parser *p)
{
  i32 expr;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  expr = ParseExpr(p);
  if (IsError(expr)) return expr;
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  Spacing(p);
  return expr;
}

#define TrivialDoNode(node)   (RawVal(NodeValue(NodeChild(node, 0))) == 0 && NodeCount(node) == 2)
val ParseDo(Parser *p)
{
  i32 stmts, pos = p->token.pos;
  if (!MatchToken(doToken, p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  if (TrivialDoNode(stmts)) return NodeChild(stmts, 1);
  return MakeNode(doNode, pos, NodeValue(stmts));
}

val ParseIf(Parser *p)
{
  i32 pred, cons, alt;
  i32 start = p->token.pos, pos;
  if (!MatchToken(ifToken, p)) return ParseError("Expected \"if\"", p);
  Spacing(p);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  pos = p->token.pos;
  if (!MatchToken(doToken, p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  cons = ParseStmts(p);
  if (IsError(cons)) return cons;
  if (TrivialDoNode(cons)) cons = NodeChild(cons, 1);
  pos = p->token.pos;
  if (MatchToken(elseToken, p)) {
    VSpacing(p);
    alt = ParseStmts(p);
    if (IsError(alt)) return alt;
    if (TrivialDoNode(alt)) alt = NodeChild(alt, 1);
  } else {
    alt = MakeNode(nilNode, pos, 0);
  }
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return MakeNode(ifNode, start, Pair(pred, Pair(cons, Pair(alt, 0))));
}

val ParseCond(Parser *p)
{
  i32 expr;
  if (!MatchToken(condToken, p)) return ParseError("Expected \"cond\"", p);
  expr = ParseClauses(p);
  if (IsError(expr)) return expr;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return expr;
}

val ParseClauses(Parser *p)
{
  i32 pred, cons, alt, pos = p->token.pos;
  VSpacing(p);
  if (CheckToken(endToken, p)) return MakeNode(nilNode, p->token.pos, 0);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  if (!MatchToken(arrowToken, p)) return ParseError("Expected \"->\"", p);
  VSpacing(p);
  cons = ParseExpr(p);
  if (IsError(cons)) return cons;
  alt = ParseClauses(p);
  if (IsError(alt)) return alt;
  return MakeNode(ifNode, pos, Pair(pred, Pair(cons, Pair(alt, 0))));
}

val ParseList(Parser *p)
{
  i32 pos = p->token.pos;
  i32 items = 0;
  if (!MatchToken(lbracketToken, p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (!MatchToken(rbracketToken, p)) {
    items = ParseItems(p);
    if (IsError(items)) return items;
    if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  }
  Spacing(p);
  return MakeNode(listNode, pos, NodeValue(items));
}

val ParseTuple(Parser *p)
{
  i32 pos = p->token.pos, items = 0;
  if (!MatchToken(lbraceToken, p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (!MatchToken(rbraceToken, p)) {
    items = ParseItems(p);
    if (IsError(items)) return items;
    if (!MatchToken(rbraceToken, p)) return ParseError("Expected \"}\"", p);
  }
  Spacing(p);
  return MakeNode(tupleNode, pos, ReverseList(NodeValue(items), 0));
}

val ParseNum(Parser *p)
{
  i32 n = 0;
  u32 i;
  char *lexeme = p->text + p->token.pos;
  if (!CheckToken(numToken, p)) return ParseError("Expected number", p);
  for (i = 0; i < p->token.length; i++) {
    i32 d = lexeme[i] - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
  }
  Adv(p);
  Spacing(p);
  return MakeNode(intNode, p->token.pos, IntVal(n));
}

val ParseByte(Parser *p)
{
  u8 byte;
  i32 pos = p->token.pos;
  char *lexeme = p->text + p->token.pos;
  if (IsSpace(lexeme[1]) || !IsPrintable(lexeme[1])) return ParseError("Expected character", p);
  byte = *lexeme;
  Adv(p);
  Spacing(p);
  return MakeNode(intNode, pos, IntVal(byte));
}

val ParseHex(Parser *p)
{
  i32 n = 0, pos = p->token.pos;
  u32 i;
  char *lexeme = p->text + p->token.pos;
  for (i = 2; i < p->token.length; i++) {
    i32 d = IsDigit(lexeme[i]) ? lexeme[i] - '0' : lexeme[i] - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
  }
  Adv(p);
  Spacing(p);
  return MakeNode(intNode, pos, IntVal(n));
}

val ParseSymbol(Parser *p)
{
  i32 sym, pos = p->token.pos;
  if (!MatchToken(colonToken, p)) return ParseError("Expected \":\"", p);
  if (!CheckToken(idToken, p))return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + p->token.pos, p->token.length);
  Adv(p);
  Spacing(p);
  return MakeNode(symNode, pos, SymVal(sym));
}

val ParseString(Parser *p)
{
  i32 sym, pos = p->token.pos;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  sym = SymbolFrom(p->text + pos + 1, p->token.length - 2);
  Adv(p);
  Spacing(p);
  return MakeNode(strNode, pos, SymVal(sym));
}

val ParseID(Parser *p)
{
  i32 sym, pos = p->token.pos;
  if (!CheckToken(idToken, p)) return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + p->token.pos, p->token.length);
  Adv(p);
  Spacing(p);
  return MakeNode(idNode, pos, SymVal(sym));
}

val ParseIDList(Parser *p)
{
  i32 items = 0;
  i32 pos = p->token.pos;
  i32 id = ParseID(p);
  if (IsError(id)) return id;
  items = Pair(id, items);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    items = Pair(id, items);
  }
  return MakeNode(listNode, pos, ReverseList(items, 0));
}

val ParseItems(Parser *p)
{
  i32 items = 0;
  i32 item = ParseExpr(p);
  i32 pos = p->token.pos;
  if (IsError(item)) return item;
  items = Pair(item, items);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsError(item)) return item;
    items = Pair(item, items);
  }
  return MakeNode(listNode, pos, items);
}

void Spacing(Parser *p)
{
  MatchToken(spaceToken, p);
}

void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) {
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
  case sliceNode:   return "slice";
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
    printf("nil:%d\n", NodePos(node));
    return;
  }

  printf("%s:%d", NodeName(type), NodePos(node));
  switch (type) {
  case idNode:
    printf(" %s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case strNode:
    printf(" \"%s\"\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case symNode:
    printf(" :%s\n", SymbolName(RawVal(NodeValue(node))));
    break;
  case intNode:
    printf(" %d\n", RawVal(NodeValue(node)));
    break;
  default:
    subnodes = NodeValue(node);
    printf("\n");
    lines |= Bit(level);
    while (subnodes && IsPair(subnodes)) {
      for (i = 0; i < level; i++) {
        if (lines & Bit(i)) {
          printf("│ ");
        } else {
          printf("  ");
        }
      }
      if (Tail(subnodes)) {
        printf("├ ");
      } else {
        lines &= ~Bit(level);
        printf("└ ");
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
