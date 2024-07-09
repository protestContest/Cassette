#include "parse.h"
#include "lex.h"
#include "types.h"
#include <univ/str.h>
#include <univ/symbol.h>
#include <univ/math.h>

typedef struct {
  char *text;
  Token token;
} Parser;
#define Adv(p) \
  (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

bool Lookahead(i32 n, Parser *p)
{
  TokenType type;
  Token saveToken = p->token;
  while (n > 0) {
    Adv(p);
    n--;
  }
  type = p->token.type;
  p->token = saveToken;
  return type;
}

val MakeNode(i32 type, i32 start, i32 end, val value)
{
  val node = Tuple(5);
  TupleSet(node, 0, IntVal(type));
  TupleSet(node, 1, value);
  TupleSet(node, 2, IntVal(start));
  TupleSet(node, 3, IntVal(end));
  TupleSet(node, 4, 0);
  return node;
}
#define ParseError(msg, p)  MakeError(msg, (p)->token.pos, (p)->token.pos + (p)->token.length)
#define TokenNode(type, token, value) \
  MakeNode(type, (token).pos, (token).pos+(token).length, value)

val ParseModule(Parser *p);
val ParseModname(Parser *p);
val ParseExports(Parser *p);
val ParseImports(Parser *p);
val ParseStmts(Parser *p);
val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p);
val ParseDef(Parser *p);
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
val ParseVar(Parser *p);
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
  i32 pos = p->token.pos;
  val modname, exports, imports, stmts;
  VSpacing(p);
  if (p->token.type == moduleToken) {
    modname = ParseModname(p);
    if (IsError(modname)) return modname;
    if (p->token.type == exportsToken) {
      exports = ParseExports(p);
      if (IsError(exports)) return exports;
    } else {
      exports = TokenNode(exportNode, p->token, 0);
    }
  } else {
    modname = TokenNode(idNode, p->token, SymVal(Symbol("*main*")));
    exports = TokenNode(exportNode, p->token, 0);
  }
  imports = ParseImports(p);
  if IsError(imports) return imports;
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  return MakeNode(moduleNode, pos, NodeEnd(stmts),
      Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

val ParseModname(Parser *p)
{
  i32 id;
  if (!MatchToken(moduleToken, p)) return ParseError("Expected \"module\"", p);
  Spacing(p);
  id = ParseID(p);
  if (IsError(id)) return id;
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Expected newline", p);
  }
  VSpacing(p);
  return id;
}

val ParseExports(Parser *p)
{
  i32 exports, pos = p->token.pos, end = 0;
  if (!MatchToken(exportsToken, p)) {
    return ParseError("Expected \"exports\"", p);
  }
  Spacing(p);
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Expected newline", p);
  }
  end = p->token.pos;
  VSpacing(p);
  return MakeNode(exportNode, pos, end, NodeValue(exports));
}

val ParseImports(Parser *p)
{
  i32 imports = 0, start = p->token.pos, pos = start;
  while (MatchToken(importToken, p)) {
    i32 import, id, alias;
    Spacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    if (MatchToken(asToken, p)) {
      Spacing(p);
      alias = ParseID(p);
      if (IsError(alias)) return alias;
    } else {
      alias = id;
    }
    if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
      return ParseError("Expected newline", p);
    }
    import = MakeNode(listNode, pos, p->token.pos, Pair(id, Pair(alias, 0)));
    imports = Pair(import, imports);
    VSpacing(p);
    pos = p->token.pos;
  }
  return MakeNode(importNode, start, NodeEnd(Head(imports)),
      ReverseList(imports, 0));
}

val ParseStmts(Parser *p)
{
  val stmts = 0, defs = 0, result;
  i32 pos = p->token.pos, numAssigns = 0;
  result = ParseStmt(&stmts, &defs, &numAssigns, p);
  if (IsError(result)) return result;
  while (!AtEnd(p) && CheckToken(newlineToken, p)) {
    VSpacing(p);
    if (AtEnd(p) || CheckToken(endToken, p) || CheckToken(elseToken, p)) break;
    result = ParseStmt(&stmts, &defs, &numAssigns, p);
    if (IsError(result)) return result;
  }
  VSpacing(p);
  stmts = ReverseList(stmts, 0);
  stmts = ReverseList(defs, stmts);
  return MakeNode(doNode, pos, p->token.pos,
      Pair(MakeNode(intNode, pos, pos, IntVal(numAssigns)), stmts));
}

val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p)
{
  val stmt, assign;
  if (CheckToken(defToken, p)) {
    stmt = ParseDef(p);
    if (IsError(stmt)) return stmt;
    *defs = Pair(stmt, *defs);
    (*numAssigns)++;
  } else if (MatchToken(letToken, p)) {
    Spacing(p);
    assign = ParseAssign(p);
    if (IsError(assign)) return assign;
    *stmts = Pair(assign, *stmts);
    (*numAssigns)++;
    while (MatchToken(commaToken, p)) {
      VSpacing(p);
      assign = ParseAssign(p);
      if (IsError(assign)) return assign;
      *stmts = Pair(assign, *stmts);
      (*numAssigns)++;
    }
  } else {
    stmt = ParseExpr(p);
    if (IsError(stmt)) return stmt;
    *stmts = Pair(stmt, *stmts);
  }
  return 0;
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
  lambda = MakeNode(lambdaNode, pos, NodeEnd(body),
      Pair(params, Pair(body, 0)));
  return MakeNode(defNode, pos, NodeEnd(lambda), Pair(id, Pair(lambda, 0)));
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
  return MakeNode(letNode, pos, NodeEnd(value), Pair(id, Pair(value, 0)));
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
  return MakeNode(lambdaNode, pos, NodeEnd(body), Pair(params, Pair(body, 0)));
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
    expr = MakeNode(op, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
    op = MatchToken(andToken, p)
      ? andNode
      : MatchToken(orToken, p)
        ? orNode
        : 0;
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
    return MakeNode(eqNode, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
  } else if (MatchToken(bangeqToken, p)) {
    i32 arg;
    VSpacing(p);
    arg = ParsePair(p);
    if (IsError(arg)) return arg;
    return MakeNode(notNode, pos, NodeEnd(arg),
        Pair(MakeNode(eqNode, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0))), 0));
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
    expr = MakeNode(pairNode, pos, NodeEnd(arg), Pair(arg, Pair(expr, 0)));
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
    expr = MakeNode(joinNode, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
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
    expr = MakeNode(op, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
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
    if (right) {
      arg = MakeNode(negNode, NodeStart(arg), NodeEnd(arg), Pair(arg, 0));
    }
    expr = MakeNode(op, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
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
    expr = MakeNode(op, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
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
    expr = MakeNode(op, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
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
    return MakeNode(negNode, pos, NodeEnd(expr), Pair(expr, 0));
  } else if (MatchToken(tildeToken, p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(compNode, pos, NodeEnd(expr), Pair(expr, 0));
  } else if (MatchToken(hashToken, p)) {
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(lenNode, pos, NodeEnd(expr), Pair(expr, 0));
  } else if (MatchToken(notToken, p)) {
    Spacing(p);
    expr = ParseCall(p);
    if (IsError(expr)) return expr;
    return MakeNode(notNode, pos, NodeEnd(expr), Pair(expr, 0));
  } else {
    return ParseCall(p);
  }
}

val ParseCall(Parser *p)
{
  i32 pos, endPos;
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
      expr = MakeNode(callNode, pos, NodeEnd(Head(NodeValue(args))),
          Pair(expr, ReverseList(NodeValue(args), 0)));
    } else if (MatchToken(lbracketToken, p)) {
      val start;
      VSpacing(p);
      start = ParseExpr(p);
      if (IsError(start)) return start;
      VSpacing(p);
      if (MatchToken(colonToken, p)) {
        val end;
        if (CheckToken(rbracketToken, p)) {
          end = MakeNode(nilNode, p->token.pos, p->token.pos, 0);
        } else {
          end = ParseExpr(p);
        }
        if (IsError(end)) return end;
        endPos = p->token.pos + p->token.length;
        if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
        expr = MakeNode(sliceNode, pos, endPos, Pair(expr, Pair(start, Pair(end, 0))));
        VSpacing(p);
      } else {
        endPos = p->token.pos + p->token.length;
        if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
        expr = MakeNode(sliceNode, pos, endPos, Pair(expr, Pair(start, 0)));
      }
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
    expr = MakeNode(accessNode, pos, NodeEnd(arg), Pair(expr, Pair(arg, 0)));
    pos = p->token.pos;
  }
  return expr;
}

val ParsePrimary(Parser *p)
{
  i32 pos = p->token.pos;
  i32 endPos = pos + p->token.length;
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
    return MakeNode(nilNode, pos, endPos, 0);
  } else if (MatchToken(trueToken, p)) {
    return MakeNode(intNode, pos, endPos, IntVal(1));
  } else if (MatchToken(falseToken, p)) {
    return MakeNode(intNode, pos, endPos, IntVal(0));
  } else if (CheckToken(idToken, p)) {
    return ParseVar(p);
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
  i32 stmts, pos = p->token.pos, endPos;
  if (!MatchToken(doToken, p)) return ParseError("Expected \"do\"", p);
  VSpacing(p);
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  endPos = p->token.pos;
  Spacing(p);
  if (TrivialDoNode(stmts)) return NodeChild(stmts, 1);
  return MakeNode(doNode, pos, endPos, NodeValue(stmts));
}

val ParseIf(Parser *p)
{
  i32 pred, cons, alt;
  i32 start = p->token.pos, pos, endPos;
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
    alt = MakeNode(nilNode, pos, pos, 0);
  }
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  endPos = p->token.pos;
  Spacing(p);
  return MakeNode(ifNode, start, endPos, Pair(pred, Pair(cons, Pair(alt, 0))));
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
  i32 pred, cons, alt;
  VSpacing(p);
  if (CheckToken(endToken, p)) return TokenNode(nilNode, p->token, 0);
  pred = ParseExpr(p);
  if (IsError(pred)) return pred;
  if (!MatchToken(arrowToken, p)) return ParseError("Expected \"->\"", p);
  VSpacing(p);
  cons = ParseExpr(p);
  if (IsError(cons)) return cons;
  alt = ParseClauses(p);
  if (IsError(alt)) return alt;
  return MakeNode(ifNode, NodeStart(pred), NodeEnd(cons), Pair(pred, Pair(cons, Pair(alt, 0))));
}

val ParseList(Parser *p)
{
  i32 pos = p->token.pos, endPos;
  i32 items = 0;
  if (!MatchToken(lbracketToken, p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (!MatchToken(rbracketToken, p)) {
    items = ParseItems(p);
    if (IsError(items)) return items;
    if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  }
  endPos = p->token.pos;
  Spacing(p);
  return MakeNode(listNode, pos, endPos, NodeValue(items));
}

val ParseTuple(Parser *p)
{
  i32 pos = p->token.pos, items = 0, endPos;
  if (!MatchToken(lbraceToken, p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  if (!MatchToken(rbraceToken, p)) {
    items = ParseItems(p);
    if (IsError(items)) return items;
    if (!MatchToken(rbraceToken, p)) return ParseError("Expected \"}\"", p);
  }
  endPos = p->token.pos;
  Spacing(p);
  return MakeNode(tupleNode, pos, endPos, ReverseList(NodeValue(items), 0));
}

val ParseNum(Parser *p)
{
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  if (!CheckToken(numToken, p)) return ParseError("Expected number", p);
  for (i = 0; i < token.length; i++) {
    i32 d = lexeme[i] - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
  }
  Adv(p);
  Spacing(p);
  return TokenNode(intNode, token, IntVal(n));
}

val ParseByte(Parser *p)
{
  u8 byte;
  i32 pos = p->token.pos, endPos = p->token.pos + p->token.length;
  char *lexeme = p->text + p->token.pos;
  if (IsSpace(lexeme[1]) || !IsPrintable(lexeme[1])) return ParseError("Expected character", p);
  byte = *lexeme;
  Adv(p);
  Spacing(p);
  return MakeNode(intNode, pos, endPos, IntVal(byte));
}

val ParseHex(Parser *p)
{
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  for (i = 2; i < token.length; i++) {
    i32 d = IsDigit(lexeme[i]) ? lexeme[i] - '0' : lexeme[i] - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
  }
  Adv(p);
  Spacing(p);
  return TokenNode(intNode, token, IntVal(n));
}

val ParseSymbol(Parser *p)
{
  i32 sym;
  Token token;
  if (!MatchToken(colonToken, p)) return ParseError("Expected \":\"", p);
  if (!CheckToken(idToken, p))return ParseError("Expected identifier", p);
  token = p->token;
  sym = SymbolFrom(p->text + token.pos, token.length);
  Adv(p);
  Spacing(p);
  return TokenNode(symNode, token, SymVal(sym));
}

val ParseString(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  sym = SymbolFrom(p->text + token.pos + 1, token.length - 2);
  Adv(p);
  Spacing(p);
  return TokenNode(strNode, token, SymVal(sym));
}

val ParseVar(Parser *p)
{
  return ParseID(p);
}

val ParseID(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(idToken, p)) return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + token.pos, token.length);
  Adv(p);
  Spacing(p);
  return TokenNode(idNode, token, SymVal(sym));
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
  return MakeNode(listNode, pos, NodeEnd(Head(items)), ReverseList(items, 0));
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
  return MakeNode(listNode, pos, NodeEnd(Head(items)), items);
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
  case structNode:  return "struct";
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
  i32 valtype = NodeProp(node, typeProp);

  printf("%s[%d:%d]", NodeName(type), NodeStart(node), NodeEnd(node));
  if (valtype) {
    printf("<");
    PrintType(valtype);
    printf(">");
  }

  switch (type) {
  case nilNode:
    printf("\n");
    break;
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
    printf(" %d\n", RawInt(NodeValue(node)));
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

void PrintError(char *prefix, val node, char *source)
{
  i32 nodeStart = NodeStart(node);
  i32 nodeLen = NodeEnd(node) - nodeStart;
  i32 col = 0, i;
  i32 line = 0;
  if (!node) return;
  if (prefix) {
    fprintf(stderr, "%s%s: %s\n", ANSIRed, prefix, ErrorMsg(node));
  } else {
    fprintf(stderr, "%s%s\n", ANSIRed, ErrorMsg(node));
  }
  if (source) {
    char *start = source + nodeStart;
    char *end = source + nodeStart;
    while (start > source && !IsNewline(start[-1])) {
      col++;
      start--;
    }
    while (*end && !IsNewline(*end)) end++;
    for (i = 0; i < nodeStart; i++) {
      if (IsNewline(source[i])) line++;
    }
    fprintf(stderr, "%3d| %*.*s\n", line+1, (i32)(end-start), (i32)(end-start), start);
    fprintf(stderr, "     ");
    for (i = 0; i < col; i++) fprintf(stderr, " ");
    for (i = 0; i < nodeLen; i++) fprintf(stderr, "^");
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "%s", ANSINormal);
}
