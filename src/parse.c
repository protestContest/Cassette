#include "parse.h"
#include "error.h"
#include "lex.h"
#include <univ/math.h>
#include <univ/str.h>
#include <univ/symbol.h>

typedef struct {
  char *text;
  Token token;
} Parser;
#define Adv(p) \
  (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define ParseJump(p, i) \
  (p)->token = NextToken((p)->text, i)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

#define ParseError(msg, p) \
  MakeError(Binary(msg), \
            Pair((p)->token.pos, (p)->token.pos + (p)->token.length))
#define TokenNode(type, token, value) \
  MakeNode(type, (token).pos, (token).pos+(token).length, value)

typedef val (*ParsePrefix)(Parser *p);
typedef val (*ParseInfix)(val expr, Parser *p);
typedef enum {precNone, precExpr, precLogic, precEqual, precPair, precJoin,
  precCompare, precShift, precSum, precProduct, precCall} Precedence;
typedef struct {
  ParsePrefix prefix;
  ParseInfix infix;
  Precedence prec;
} ParseRule;

static val ParseModule(Parser *p);
static val ParseModname(Parser *p);
static val ParseExports(Parser *p);
static val ParseImports(Parser *p);
static val ParseStmts(Parser *p);
static val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p);
static val ParseDef(Parser *p);
static val ParseAssign(Parser *p);

#define ParseExpr(p)  ParsePrec(precExpr, p)
static val ParsePrec(Precedence prec, Parser *p);
static val ParseOp(val expr, Parser *p);
static val ParsePair(val expr, Parser *p);
static val ParseCall(val expr, Parser *p);
static val ParseAccess(val expr, Parser *p);
static val ParseUnary(Parser *p);
static val ParseLambda(Parser *p);
static val ParseGroup(Parser *p);
static val ParseDo(Parser *p);
static val ParseIf(Parser *p);
static val ParseCond(Parser *p);
static val ParseClauses(Parser *p);
static val ParseList(Parser *p);
static val ParseTuple(Parser *p);
static val ParseVar(Parser *p);
static val ParseNum(Parser *p);
static val ParseByte(Parser *p);
static val ParseHex(Parser *p);
static val ParseSymbol(Parser *p);
static val ParseString(Parser *p);
static val ParseLiteral(Parser *p);

static val ParseID(Parser *p);
static val ParseIDList(Parser *p);
static val ParseItems(Parser *p);
static void Spacing(Parser *p);
static void VSpacing(Parser *p);

static ParseRule rules[] = {
  [eofToken]      = {0,             0,            precNone},
  [newlineToken]  = {0,             0,            precNone},
  [spaceToken]    = {0,             0,            precNone},
  [bangeqToken]   = {0,             ParseOp,      precEqual},
  [stringToken]   = {ParseString,   0,            precNone},
  [hashToken]     = {ParseUnary,    0,            precNone},
  [byteToken]     = {ParseByte,     0,            precNone},
  [percentToken]  = {0,             ParseOp,      precProduct},
  [ampToken]      = {0,             ParseOp,      precProduct},
  [lparenToken]   = {ParseGroup,    ParseCall,    precCall},
  [rparenToken]   = {0,             0,            precNone},
  [starToken]     = {0,             ParseOp,      precProduct},
  [plusToken]     = {0,             ParseOp,      precSum},
  [commaToken]    = {0,             0,            precNone},
  [minusToken]    = {ParseUnary,    ParseOp,      precSum},
  [arrowToken]    = {0,             0,            precNone},
  [dotToken]      = {0,             0,            precNone},
  [slashToken]    = {0,             ParseOp,      precProduct},
  [numToken]      = {ParseNum,      0,            precNone},
  [hexToken]      = {ParseHex,      0,            precNone},
  [colonToken]    = {ParseSymbol,   0,            precNone},
  [ltToken]       = {0,             ParseOp,      precCompare},
  [ltltToken]     = {0,             ParseOp,      precShift},
  [lteqToken]     = {0,             ParseOp,      precCompare},
  [ltgtToken]     = {0,             ParseOp,      precJoin},
  [eqToken]       = {0,             0,            precNone},
  [eqeqToken]     = {0,             ParseOp,      precEqual},
  [gtToken]       = {0,             ParseOp,      precCompare},
  [gteqToken]     = {0,             ParseOp,      precCompare},
  [gtgtToken]     = {0,             ParseOp,      precShift},
  [idToken]       = {ParseVar,      0,            precNone},
  [andToken]      = {0,             ParseOp,      precLogic},
  [asToken]       = {0,             0,            precNone},
  [condToken]     = {ParseCond,     0,            precNone},
  [defToken]      = {0,             0,            precNone},
  [doToken]       = {ParseDo,       0,            precNone},
  [elseToken]     = {0,             0,            precNone},
  [endToken]      = {0,             0,            precNone},
  [exportsToken]  = {0,             0,            precNone},
  [falseToken]    = {ParseLiteral,  0,            precNone},
  [ifToken]       = {ParseIf,       0,            precNone},
  [importToken]   = {0,             0,            precNone},
  [letToken]      = {0,             0,            precNone},
  [moduleToken]   = {0,             0,            precNone},
  [nilToken]      = {ParseLiteral,  0,            precNone},
  [notToken]      = {ParseUnary,    0,            precNone},
  [orToken]       = {0,             ParseOp,      precLogic},
  [trueToken]     = {ParseLiteral,  0,            precNone},
  [lbraceToken]   = {ParseTuple,    0,            precNone},
  [bslashToken]   = {ParseLambda,   0,            precNone},
  [rbraceToken]   = {0,             0,            precNone},
  [caretToken]    = {0,             ParseOp,      precSum},
  [lbracketToken] = {ParseList,     ParseAccess,  precCall},
  [barToken]      = {0,             ParsePair,    precPair},
  [rbracketToken] = {0,             0,            precNone},
  [tildeToken]    = {ParseUnary,    0,            precNone},
  [errorToken]    = {0,             0,            precNone}
};

val Parse(char *text)
{
  Parser p;
  p.text = text;
  p.token.pos = 0;
  p.token.length = 0;
  Adv(&p);
  return ParseModule(&p);
}

static val ParseModule(Parser *p)
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
      exports = TokenNode(listNode, p->token, 0);
    }
  } else {
    modname = TokenNode(idNode, p->token, SymVal(Symbol("*main*")));
    exports = TokenNode(listNode, p->token, 0);
  }
  imports = ParseImports(p);
  if IsError(imports) return imports;
  stmts = ParseStmts(p);
  if (IsError(stmts)) return stmts;
  if (!AtEnd(p)) return ParseError("Expected end of file", p);
  return MakeNode(moduleNode, pos, NodeEnd(stmts),
      Pair(modname, Pair(exports, Pair(imports, Pair(stmts, 0)))));
}

static val ParseModname(Parser *p)
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

static val ParseExports(Parser *p)
{
  i32 exports, pos = p->token.pos;
  if (!MatchToken(exportsToken, p)) {
    return ParseError("Expected \"exports\"", p);
  }
  Spacing(p);
  exports = ParseIDList(p);
  if (IsError(exports)) return exports;
  if (!AtEnd(p) && !MatchToken(newlineToken, p)) {
    return ParseError("Missing comma or newline", p);
  }
  VSpacing(p);
  return MakeNode(listNode, pos, NodeEnd(exports), NodeValue(exports));
}

static val ParseImports(Parser *p)
{
  i32 imports = 0, start = p->token.pos, pos = p->token.pos;
  while (MatchToken(importToken, p)) {
    i32 import, id, alias;
    Spacing(p);
    id = ParseID(p);
    if (IsError(id)) return id;
    Spacing(p);
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
    VSpacing(p);
    import = MakeNode(importNode, pos, NodeEnd(alias),
        Pair(id, Pair(alias, 0)));
    imports = Pair(import, imports);
    pos = p->token.pos;
  }
  return MakeNode(listNode, start, NodeEnd(Head(imports)),
      ReverseList(imports, 0));
}

static val ParseStmts(Parser *p)
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
  if (!stmts || NodeType(Head(stmts)) == letNode || NodeType(Head(stmts)) == defNode) {
    stmts = Pair(MakeNode(nilNode, p->token.pos, p->token.pos, 0), stmts);
  }
  VSpacing(p);
  stmts = ReverseList(stmts, 0);
  stmts = ReverseList(defs, stmts);
  return MakeNode(doNode, pos, p->token.pos,
      Pair(MakeNode(intNode, pos, pos, IntVal(numAssigns)), stmts));
}

static val ParseStmt(val *stmts, val *defs, i32 *numAssigns, Parser *p)
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

static val ParseDef(Parser *p)
{
  val id, lambda, params = 0, body;
  i32 pos = p->token.pos;
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

static val ParseAssign(Parser *p)
{
  val id, value;
  id = ParseID(p);
  if (IsError(id)) return id;
  Spacing(p);
  if (!MatchToken(eqToken, p)) return ParseError("Expected \"=\"", p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsError(value)) return value;
  return MakeNode(letNode, NodeStart(id), NodeEnd(value),
      Pair(id, Pair(value, 0)));
}

static val ParsePrec(Precedence prec, Parser *p)
{
  val expr;
  ParseRule rule = rules[p->token.type];
  if (!rule.prefix) return ParseError("Unexpected token", p);
  expr = rule.prefix(p);
  if (IsError(expr)) return expr;
  rule = rules[p->token.type];
  while (rule.prec >= prec) {
    expr = rule.infix(expr, p);
    if (IsError(expr)) return expr;
    rule = rules[p->token.type];
  }
  return expr;
}

i32 OpNodeType(TokenType type)
{
  switch (type) {
  case bangeqToken:   return eqNode;
  case percentToken:  return remNode;
  case ampToken:      return bitandNode;
  case starToken:     return mulNode;
  case plusToken:     return addNode;
  case minusToken:    return subNode;
  case slashToken:    return divNode;
  case ltToken:       return ltNode;
  case ltltToken:     return shiftNode;
  case lteqToken:     return gtNode;
  case ltgtToken:     return joinNode;
  case eqeqToken:     return eqNode;
  case gtToken:       return gtNode;
  case gteqToken:     return ltNode;
  case andToken:      return andNode;
  case orToken:       return orNode;
  case lbracketToken: return sliceNode;
  case caretToken:    return bitorNode;
  case barToken:      return pairNode;
  default:            assert(false);
  }
}

static val ParseOp(val expr, Parser *p)
{
  Token token = p->token;
  val arg, value;
  i32 start, end;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsError(arg)) return arg;
  start = NodeStart(expr);
  end = NodeEnd(arg);
  if (token.type == gtgtToken) {
    arg = MakeNode(negNode, start, end, Pair(arg, 0));
  }
  value = Pair(expr, Pair(arg, 0));
  if (token.type == bangeqToken ||
      token.type == lteqToken ||
      token.type == gteqToken) {
    value = Pair(MakeNode(OpNodeType(token.type), start, end, value), 0);
    return MakeNode(notNode, start, end, value);
  }
  return MakeNode(OpNodeType(token.type), start, end, value);
}

static val ParsePair(val expr, Parser *p)
{
  Token token = p->token;
  val arg, value;
  i32 start, end;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec, p);
  if (IsError(arg)) return arg;
  start = NodeStart(expr);
  end = NodeEnd(arg);
  value = Pair(expr, Pair(arg, 0));
  return MakeNode(pairNode, start, end, value);
}

static val ParseCall(val expr, Parser *p)
{
  val args;
  i32 end;
  Adv(p);
  VSpacing(p);
  args = ParseItems(p);
  if (IsError(args)) return args;
  if (!MatchToken(rparenToken, p)) return ParseError("Expected \")\"", p);
  end = p->token.pos;
  Spacing(p);
  return MakeNode(callNode, NodeStart(expr), end,
    Pair(expr, ReverseList(NodeChildren(args), 0)));
}

static val ParseAccess(val expr, Parser *p)
{
  val startNode, endNode = 0;
  i32 end;
  Adv(p);
  VSpacing(p);
  startNode = ParseExpr(p);
  if (IsError(startNode)) return startNode;
  VSpacing(p);
  if (MatchToken(colonToken, p)) {
    VSpacing(p);
    if (CheckToken(rbracketToken, p)) {
      endNode = MakeNode(lenNode, p->token.pos, p->token.pos, Pair(expr, 0));
    } else {
      endNode = ParseExpr(p);
      if (IsError(endNode)) return endNode;
      VSpacing(p);
    }
  }
  if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  end = p->token.pos;
  if (endNode) {
    return MakeNode(sliceNode, NodeStart(expr), end,
      Pair(expr, Pair(startNode, Pair(endNode, 0))));
  } else {
    return MakeNode(accessNode, NodeStart(expr), end,
      Pair(expr, Pair(startNode, 0)));
  }
}

i32 UnaryOpNodeType(TokenType type)
{
  switch (type) {
  case minusToken: return negNode;
  case hashToken: return lenNode;
  case tildeToken: return compNode;
  case notToken: return notNode;
  default: assert(false);
  }
}

static val ParseUnary(Parser *p)
{
  Token token = p->token;
  val arg;
  Adv(p);
  if (token.type == notToken) VSpacing(p);
  arg = ParsePrec(precCall, p);
  if (IsError(arg)) return arg;
  return MakeNode(UnaryOpNodeType(token.type), token.pos, NodeEnd(arg),
    Pair(arg, 0));
}

static val ParseLambda(Parser *p)
{
  val params = 0, body;
  i32 pos = p->token.pos;
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

static val ParseGroup(Parser *p)
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

#define TrivialDoNode(node) \
    (RawVal(NodeValue(NodeChild(node, 0))) == 0 && NodeLength(node) == 2)
static val ParseDo(Parser *p)
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

static val ParseIf(Parser *p)
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

static val ParseCond(Parser *p)
{
  i32 expr;
  if (!MatchToken(condToken, p)) return ParseError("Expected \"cond\"", p);
  expr = ParseClauses(p);
  if (IsError(expr)) return expr;
  if (!MatchToken(endToken, p)) return ParseError("Expected \"end\"", p);
  Spacing(p);
  return expr;
}

static val ParseClauses(Parser *p)
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

static val ParseList(Parser *p)
{
  i32 startPos = p->token.pos, endPos;
  i32 items = 0;
  val expr;
  if (!MatchToken(lbracketToken, p)) return ParseError("Expected \"[\"", p);
  VSpacing(p);
  if (!MatchToken(rbracketToken, p)) {
    items = ParseItems(p);
    if (IsError(items)) return items;
    items = NodeValue(items);
    if (!MatchToken(rbracketToken, p)) return ParseError("Expected \"]\"", p);
  }
  endPos = p->token.pos;
  Spacing(p);

  expr = MakeNode(nilNode, endPos, endPos, 0);
  while (items) {
    val item = Head(items);
    items = Tail(items);
    expr = MakeNode(pairNode, startPos, endPos, Pair(item, Pair(expr, 0)));
  }

  return expr;
}

static val ParseTuple(Parser *p)
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

  return MakeNode(callNode, pos, endPos,
    Pair(MakeNode(idNode, pos, pos+1, SymVal(Symbol("{}"))),
         ReverseList(NodeValue(items), 0)));
}

static val ParseVar(Parser *p)
{
  return ParseID(p);
}


static val ParseNum(Parser *p)
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

static val ParseHex(Parser *p)
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

static val ParseByte(Parser *p)
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

static val ParseSymbol(Parser *p)
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

static val ParseString(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  sym = SymbolFrom(p->text + token.pos + 1, token.length - 2);
  Adv(p);
  Spacing(p);
  return TokenNode(strNode, token, SymVal(sym));
}

static val ParseLiteral(Parser *p)
{
  Token token = p->token;
  if (MatchToken(nilToken, p)) {
    Spacing(p);
    return TokenNode(nilNode, token, 0);
  } else if (MatchToken(trueToken, p)) {
    Spacing(p);
    return TokenNode(intNode, token, IntVal(1));
  } else if (MatchToken(falseToken, p)) {
    Spacing(p);
    return TokenNode(intNode, token, IntVal(0));
  } else {
    return ParseError("Unexpected expression", p);
  }
}

static val ParseID(Parser *p)
{
  i32 sym;
  Token token = p->token;
  if (!CheckToken(idToken, p)) return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + token.pos, token.length);
  Adv(p);
  Spacing(p);
  return TokenNode(idNode, token, SymVal(sym));
}

static val ParseIDList(Parser *p)
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

static val ParseItems(Parser *p)
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

static void Spacing(Parser *p)
{
  MatchToken(spaceToken, p);
}

static void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) {
    VSpacing(p);
  }
}
