#include "parse.h"
#include "lex.h"
#include "mem.h"
#include "node.h"
#include "univ/str.h"
#include "univ/symbol.h"

typedef struct {
  char *text;
  Token token;
} Parser;

typedef ASTNode* (*ParsePrefix)(Parser *p);
typedef ASTNode* (*ParseInfix)(ASTNode *expr, Parser *p);
typedef enum {
  precNone, precExpr, precLogic, precEqual, precPair, precJoin,
  precCompare, precShift, precSum, precProduct, precCall, precQualify
} Precedence;
typedef struct {
  ParsePrefix prefix;
  ParseInfix infix;
  Precedence prec;
} ParseRule;

#define Adv(p) \
  (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))
#define Lexeme(token, p)    SymbolFrom((p)->text + (token).pos, (token).length)
#define ParseFail(n,r)      (FreeNode(n), (r))
#define Spacing(p)          MatchToken(spaceToken, p)
#define MakeTerminal(type, value, p) \
  NewNode(type, (p)->token.pos, (p)->token.pos + (p)->token.length, value)
#define MakeNode(type, p)   MakeTerminal(type, 0, p)
#define NilNode(p)          MakeNode(constNode, p)
#define Expected(s, n, p) \
  ParseFail(n, ParseError("Expected \"" s "\"", p))

static void InitParser(Parser *p, char *text)
{
  p->text = text;
  p->token.pos = 0;
  p->token.length = 0;
  Adv(p);
}

static ASTNode *ParseError(char *msg, Parser *p)
{
  char *error = NewString("Parse error: ^");
  error = FormatString(error, msg);
  return MakeTerminal(errorNode, Symbol(error), p);
}

static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

static ASTNode *ParseModuleName(Parser *p);
static ASTNode *ParseImports(Parser *p);
static ASTNode *ParseImportList(Parser *p);
static ASTNode *ParseImport(Parser *p);
static ASTNode *ParseAlias(Parser *p);
static ASTNode *ParseFnList(Parser *p);
static ASTNode *ParseExports(Parser *p);
static ASTNode *ParseStmts(Parser *p);
static ASTNode *ParseStmt(Parser *p);
static ASTNode *ParseDef(Parser *p);
static ASTNode *ParseParams(Parser *p);
static ASTNode *ParseDefGuard(Parser *p);
#define ParseExpr(p)  ParsePrec(precExpr, p)
static ASTNode *ParsePrec(Precedence prec, Parser *p);
static ASTNode *ParseOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNegOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNotOp(ASTNode *expr, Parser *p);
static ASTNode *ParsePair(ASTNode *expr, Parser *p);
static ASTNode *ParseCall(ASTNode *expr, Parser *p);
static ASTNode *ParseAccess(ASTNode *expr, Parser *p);
static ASTNode *ParseUnary(Parser *p);
static ASTNode *ParseLet(Parser *p);
static ASTNode *ParseAssigns(Parser *p);
static ASTNode *ParseAssign(Parser *p);
static ASTNode *ParseLetGuard(Parser *p);


static ASTNode *ParseItems(Parser *p);
static ASTNode *ParseIDList(Parser *p);
static ASTNode *ParseID(Parser *p);
static void VSpacing(Parser *p);






static ASTNode *ParseLambda(Parser *p);
static ASTNode *ParseGroup(Parser *p);
static ASTNode *ParseDo(Parser *p);
static ASTNode *ParseIf(Parser *p);
static ASTNode *ParseCond(Parser *p);
static ASTNode *ParseClauses(Parser *p);
static ASTNode *ParseList(Parser *p);
static ASTNode *ParseListTail(Parser *p);
static ASTNode *ParseTuple(Parser *p);
static ASTNode *ParseNum(Parser *p);
static ASTNode *ParseByte(Parser *p);
static ASTNode *ParseHex(Parser *p);
static ASTNode *ParseSymbol(Parser *p);
static ASTNode *ParseString(Parser *p);
static ASTNode *ParseLiteral(Parser *p);

static ASTNode *ParseParams(Parser *p);

static ParseRule rules[] = {
  [eofToken]      = {0,             0,            precNone},
  [newlineToken]  = {0,             0,            precNone},
  [spaceToken]    = {0,             0,            precNone},
  [bangeqToken]   = {0,             ParseNotOp,   precEqual},
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
  [dotToken]      = {0,             ParseOp,      precQualify},
  [slashToken]    = {0,             ParseOp,      precProduct},
  [numToken]      = {ParseNum,      0,            precNone},
  [hexToken]      = {ParseHex,      0,            precNone},
  [colonToken]    = {ParseSymbol,   ParsePair,    precPair},
  [ltToken]       = {0,             ParseOp,      precCompare},
  [ltltToken]     = {0,             ParseOp,      precShift},
  [lteqToken]     = {0,             ParseNotOp,   precCompare},
  [ltgtToken]     = {0,             ParseOp,      precJoin},
  [eqToken]       = {0,             0,            precNone},
  [eqeqToken]     = {0,             ParseOp,      precEqual},
  [gtToken]       = {0,             ParseOp,      precCompare},
  [gteqToken]     = {0,             ParseNotOp,   precCompare},
  [gtgtToken]     = {0,             ParseNegOp,   precShift},
  [atToken]       = {ParseUnary,    0,            precNone},
  [idToken]       = {ParseID,       0,            precNone},
  [andToken]      = {0,             ParseOp,      precLogic},
  [asToken]       = {0,             0,            precNone},
  [defToken]      = {0,             0,            precNone},
  [doToken]       = {ParseDo,       0,            precNone},
  [elseToken]     = {0,             0,            precNone},
  [endToken]      = {0,             0,            precNone},
  [exportToken]   = {0,             0,            precNone},
  [falseToken]    = {ParseLiteral,  0,            precNone},
  [ifToken]       = {ParseIf,       0,            precNone},
  [importToken]   = {0,             0,            precNone},
  [letToken]      = {ParseLet,      0,            precNone},
  [moduleToken]   = {0,             0,            precNone},
  [nilToken]      = {ParseLiteral,  0,            precNone},
  [notToken]      = {ParseUnary,    0,            precNone},
  [orToken]       = {0,             ParseOp,      precLogic},
  [trueToken]     = {ParseLiteral,  0,            precNone},
  [lbraceToken]   = {ParseTuple,    0,            precNone},
  [bslashToken]   = {ParseLambda,   0,            precNone},
  [rbraceToken]   = {0,             0,            precNone},
  [caretToken]    = {ParseUnary,    0,            precNone},
  [lbracketToken] = {ParseList,     ParseAccess,  precCall},
  [barToken]      = {0,             ParseOp,      precSum},
  [rbracketToken] = {0,             0,            precNone},
  [tildeToken]    = {ParseUnary,    0,            precNone},
  [errorToken]    = {0,             0,            precNone}
};

ASTNode *ParseModule(char *source)
{
  Parser p;
  ASTNode *node, *result;
  InitParser(&p, source);

  VSpacing(&p);
  node = MakeNode(moduleNode, &p);

  result = ParseModuleName(&p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);

  result = ParseImports(&p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);

  VSpacing(&p);
  result = ParseExports(&p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);

  VSpacing(&p);
  result = ParseStmts(&p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);

  if (!AtEnd(&p)) return Expected("End of file", node, &p);
  node->end = p.token.pos;

  return node;
}

static ASTNode *ParseModuleName(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(moduleToken, p)) return NilNode(p);
  Spacing(p);
  node = ParseID(p);
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *ParseImports(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(importToken, p)) return MakeNode(listNode, p);
  VSpacing(p);
  node = ParseImportList(p);
  if (IsErrorNode(node)) return node;
  node->end = p->token.pos;
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *ParseImportList(Parser *p)
{
  ASTNode *node, *import;
  node = MakeNode(listNode, p);
  do {
    VSpacing(p);
    import = ParseImport(p);
    if (IsErrorNode(import)) return ParseFail(node, import);
    NodePush(node, import);
  } while (MatchToken(commaToken, p));
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseImport(Parser *p)
{
  ASTNode *node, *result;
  node = MakeNode(importNode, p);

  result = ParseID(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  Spacing(p);
  result = ParseAlias(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  Spacing(p);
  result = ParseFnList(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseAlias(Parser *p)
{
  if (!MatchToken(asToken, p)) return NilNode(p);
  Spacing(p);
  return ParseID(p);
}

static ASTNode *ParseFnList(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(lparenToken, p)) return MakeNode(listNode, p);
  node = ParseIDList(p);
  if (IsErrorNode(node)) return node;
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  node->end =  p->token.pos;
  return node;
}

static ASTNode *ParseExports(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(exportToken, p)) return MakeNode(listNode, p);
  VSpacing(p);
  node = ParseIDList(p);
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *ParseStmts(Parser *p)
{
  ASTNode *node, *stmt;
  node = MakeNode(doNode, p);

  stmt = ParseStmt(p);
  if (IsErrorNode(stmt)) return ParseFail(node, stmt);
  NodePush(node, stmt);
  while (!AtEnd(p) && CheckToken(newlineToken, p)) {
    VSpacing(p);
    if (AtEnd(p) || CheckToken(endToken, p)) break;
    stmt = ParseStmt(p);
    if (IsErrorNode(stmt)) return ParseFail(node, stmt);
    NodePush(node, stmt);
  }
  return node;
}

static ASTNode *ParseStmt(Parser *p)
{
  if (CheckToken(defToken, p)) {
    return ParseDef(p);
  } else {
    return ParseExpr(p);
  }
}

static ASTNode *ParseDef(Parser *p)
{
  ASTNode *node, *result;
  node = MakeNode(defNode, p);
  MatchToken(defToken, p);
  Spacing(p);
  result = ParseID(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  if (!MatchToken(lparenToken, p)) return Expected("(", node, p);
  VSpacing(p);
  result = ParseParams(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  VSpacing(p);
  result = ParseDefGuard(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  return node;
}

static ASTNode *ParseParams(Parser *p)
{
  if (CheckToken(idToken, p)) return ParseIDList(p);
  return MakeNode(listNode, p);
}

static ASTNode *ParseDefGuard(Parser *p)
{
  return NilNode(p);
  /*
  if (!MatchToken(whenToken, p)) return NilNode(p);
  VSpacing(p);
  return ParseExpr(p);
  */
}

static ASTNode *ParsePrec(Precedence prec, Parser *p)
{
  ASTNode *expr;
  ParseRule rule = rules[p->token.type];
  if (!rule.prefix) return ParseError("Unexpected token", p);
  expr = rule.prefix(p);
  if (IsErrorNode(expr)) return expr;
  Spacing(p);
  rule = rules[p->token.type];
  while (rule.prec >= prec) {
    expr = rule.infix(expr, p);
    if (IsErrorNode(expr)) return expr;
    Spacing(p);
    rule = rules[p->token.type];
  }
  return expr;
}

NodeType OpNodeType(TokenType type)
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
  case barToken:      return bitorNode;
  case colonToken:    return pairNode;
  case dotToken:      return refNode;
  default:            assert(false);
  }
}

static ASTNode *ParseOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(OpNodeType(token.type), p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, arg);
  return node;
}

static ASTNode *ParseNegOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node, *neg;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(OpNodeType(token.type), p);
  neg = MakeNode(negNode, p);
  NodePush(neg, arg);

  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, neg);
  return node;
}

static ASTNode *ParseNotOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node, *not;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(OpNodeType(token.type), p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, arg);

  not = MakeNode(notNode, p);
  NodePush(not, node);
  return not;
}

static ASTNode *ParsePair(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);
  node = MakeNode(OpNodeType(token.type), p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, arg);
  NodePush(node, expr);
  return node;
}

static ASTNode *ParseCall(ASTNode *expr, Parser *p)
{
  ASTNode *node, *args;
  Adv(p);
  node = MakeNode(callNode, p);
  node->start = expr->start;
  NodePush(node, expr);
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) {
    args = ParseItems(p);
    if (IsErrorNode(args)) return ParseFail(node, args);
    NodePush(node, args);
    if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  } else {
    NodePush(node, MakeNode(listNode, p));
  }
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseAccess(ASTNode *expr, Parser *p)
{
  ASTNode *node, *arg;
  Adv(p);
  node = MakeNode(accessNode, p);
  node->start = expr->start;
  NodePush(node, expr);
  VSpacing(p);

  arg = ParseExpr(p);
  if (IsErrorNode(arg)) return ParseFail(node, arg);
  NodePush(node, arg);

  VSpacing(p);
  if (MatchToken(commaToken, p)) {
    node->type = sliceNode;
    VSpacing(p);
    arg = ParseExpr(p);
    if (IsErrorNode(arg)) return ParseFail(node, arg);
    NodePush(node, arg);
    VSpacing(p);
  }
  if (!MatchToken(rbracketToken, p)) return Expected("]", node, p);
  node->end = p->token.pos;
  return node;
}

static i32 UnaryOpNodeType(TokenType type)
{
  switch (type) {
  case minusToken:  return negNode;
  case hashToken:   return lenNode;
  case tildeToken:  return compNode;
  case atToken:     return headNode;
  case caretToken:  return tailNode;
  case notToken:    return notNode;
  default:          assert(false);
  }
}

static ASTNode *ParseUnary(Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  if (token.type == notToken) VSpacing(p);
  arg = ParsePrec(precCall, p);
  if (IsErrorNode(arg)) return arg;
  node = MakeNode(UnaryOpNodeType(token.type), p);
  NodePush(node, arg);
  return node;
}

static ASTNode *ParseLambda(Parser *p)
{
  ASTNode *params, *body, *node;
  node = MakeNode(lambdaNode, p);
  if (!MatchToken(bslashToken, p)) return Expected("\\", node, p);
  Spacing(p);
  params = ParseParams(p);
  if (IsErrorNode(params)) return ParseFail(node, params);
  NodePush(node, params);
  if (!MatchToken(arrowToken, p)) return Expected("->", node, p);
  VSpacing(p);
  body = ParseExpr(p);
  if (IsErrorNode(body)) return ParseFail(node, body);
  NodePush(node, body);
  return node;
}

static ASTNode *ParseGroup(Parser *p)
{
  ASTNode *node;
  if (!MatchToken(lparenToken, p)) return ParseError("Expected \"(\"", p);
  VSpacing(p);
  node = ParseExpr(p);
  if (IsErrorNode(node)) return node;
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  return node;
}

static ASTNode *ParseDo(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  if (!MatchToken(doToken, p)) return Expected("do", node, p);
  VSpacing(p);
  node = ParseStmts(p);
  if (IsErrorNode(node)) return node;
  node->start = start;
  if (!MatchToken(endToken, p)) return Expected("end", node, p);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseLet(Parser *p)
{
  ASTNode *node = 0, *assigns, *expr;
  i32 start = p->token.pos;
  if (!MatchToken(letToken, p)) return Expected("let", node, p);
  node = MakeNode(letNode, p);
  node->start = start;
  assigns = ParseAssigns(p);
  if (IsErrorNode(assigns)) return ParseFail(node, assigns);
  NodePush(node, assigns);
  VSpacing(p);
  if (!MatchToken(inToken, p)) return Expected("in", node, p);
  VSpacing(p);
  expr = ParseExpr(p);
  if (IsErrorNode(expr)) return ParseFail(node, expr);
  NodePush(node, expr);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseAssigns(Parser *p)
{
  ASTNode *node = MakeNode(listNode, p), *assign;
  do {
    VSpacing(p);
    if (CheckToken(inToken, p)) break;
    assign = ParseAssign(p);
    if (IsErrorNode(assign)) return ParseFail(node, assign);
    NodePush(node, assign);
  } while (MatchToken(newlineToken, p));
  return node;
}

static ASTNode *ParseAssign(Parser *p)
{
  ASTNode *node = MakeNode(assignNode, p), *result;
  result = ParseID(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  Spacing(p);
  if (!MatchToken(eqToken, p)) return Expected("=", node, p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  result = ParseLetGuard(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseLetGuard(Parser *p)
{
  ASTNode *node = MakeNode(listNode, p), *result;
  if (!MatchToken(exceptToken, p)) return node;
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  if (!MatchToken(elseToken, p)) return Expected("else", node, p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseIf(Parser *p)
{
  if (!MatchToken(ifToken, p)) return Expected("if", 0, p);
  VSpacing(p);
  return ParseClauses(p);
}

static ASTNode *ParseClauses(Parser *p)
{
  ASTNode *node, *result;
  if (MatchToken(elseToken, p)) {
    VSpacing(p);
    return ParseExpr(p);
  }
  node = MakeNode(ifNode, p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  if (!MatchToken(commaToken, p)) return Expected(",", node, p);
  VSpacing(p);
  result = ParseExpr(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  VSpacing(p);
  result = ParseClauses(p);
  if (IsErrorNode(result)) return ParseFail(node, result);
  NodePush(node, result);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseList(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  if (!MatchToken(lbracketToken, p)) return Expected("[", node, p);
  VSpacing(p);
  if (MatchToken(rbracketToken, p)) {
    node = NilNode(p);
  } else {
    node = ParseListTail(p);
    VSpacing(p);
    if (!MatchToken(rbracketToken, p)) return Expected("]", node, p);
  }
  node->start = start;
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseListTail(Parser *p)
{
  ASTNode *node, *item, *tail;

  item = ParseExpr(p);
  if (IsErrorNode(item)) return item;

  if (MatchToken(commaToken, p)) {
    VSpacing(p);
    tail = ParseListTail(p);
    if (IsErrorNode(tail)) return ParseFail(item, tail);
  } else {
    tail = NilNode(p);
  }

  node = MakeNode(pairNode, p);
  NodePush(node, tail);
  NodePush(node, item);
  node->end = tail->end;
  return node;
}

static ASTNode *ParseTuple(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(lbraceToken, p)) return ParseError("Expected \"{\"", p);
  VSpacing(p);
  node = ParseItems(p);
  if (IsErrorNode(node)) return node;
  node->type = tupleNode;
  node->start = start;
  if (!MatchToken(rbraceToken, p)) return Expected("}", node, p);
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseNum(Parser *p)
{
  ASTNode *node;
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  if (!CheckToken(numToken, p)) return ParseError("Expected number", p);
  for (i = 0; i < token.length; i++) {
    i32 d;
    if (lexeme[i] == '_') continue;
    d = lexeme[i] - '0';
    if (n > (MaxIntVal - d)/10) return ParseError("Integer overflow", p);
    n = n*10 + d;
  }
  node = MakeTerminal(constNode, IntVal(n), p);
  Adv(p);
  return node;
}

static ASTNode *ParseHex(Parser *p)
{
  ASTNode *node;
  i32 n = 0;
  u32 i;
  Token token = p->token;
  char *lexeme = p->text + token.pos;
  for (i = 2; i < token.length; i++) {
    i32 d;
    if (lexeme[i] == '_') continue;
    d = IsDigit(lexeme[i]) ? lexeme[i] - '0' : lexeme[i] - 'A' + 10;
    if (n > (MaxIntVal - d)/16) return ParseError("Integer overflow", p);
    n = n*16 + d;
  }
  node = MakeTerminal(constNode, IntVal(n), p);
  Adv(p);
  return node;
}

static ASTNode *ParseByte(Parser *p)
{
  ASTNode *node;
  u8 byte;
  char *lexeme = p->text + p->token.pos;
  if (IsSpace(lexeme[1]) || !IsPrintable(lexeme[1])) return ParseError("Expected character", p);
  byte = lexeme[1];
  node = MakeTerminal(constNode, IntVal(byte), p);
  Adv(p);
  return node;
}

static ASTNode *ParseSymbol(Parser *p)
{
  ASTNode *node;
  i32 sym;
  i32 start = p->token.pos;
  Token token;
  if (!MatchToken(colonToken, p)) return ParseError("Expected \":\"", p);
  token = p->token;
  sym = SymbolFrom(p->text + token.pos, token.length);
  node = MakeTerminal(constNode, IntVal(sym), p);
  node->start = start;
  node->end = token.pos + token.length;
  Adv(p);
  return node;
}

static ASTNode *ParseString(Parser *p)
{
  ASTNode *node;
  i32 sym;
  u32 i, j, len;
  char *str;
  Token token = p->token;
  if (!CheckToken(stringToken, p)) return ParseError("Expected string", p);
  for (i = 0, len = 0; i < token.length - 2; i++) {
    char ch = p->text[token.pos + i + 1];
    if (ch == '\\') i++;
    len++;
  }
  str = malloc(len+1);
  str[len] = 0;
  for (i = 0, j = 0; j < len; j++) {
    char ch = p->text[token.pos + i + 1];
    if (ch == '\\') {
      i++;
      ch = p->text[token.pos + i + 1];
    }
    str[j] = ch;
    i++;
  }
  sym = Symbol(str);
  free(str);
  node = MakeTerminal(strNode, IntVal(sym), p);
  Adv(p);
  return node;
}

static ASTNode *ParseLiteral(Parser *p)
{
  ASTNode *node;
  if (MatchToken(nilToken, p)) {
    node = NilNode(p);
  } else if (MatchToken(trueToken, p)) {
    node = MakeTerminal(constNode, IntVal(1), p);
  } else if (MatchToken(falseToken, p)) {
    node = MakeTerminal(constNode, IntVal(0), p);
  } else {
    assert(false);
  }
  return node;
}

static ASTNode *ParseItems(Parser *p)
{
  ASTNode *node = MakeNode(listNode, p);
  ASTNode *item = ParseExpr(p);
  if (IsErrorNode(item)) return ParseFail(node, item);
  NodePush(node, item);
  VSpacing(p);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    item = ParseExpr(p);
    if (IsErrorNode(item)) return ParseFail(node, item);
    NodePush(node, item);
    VSpacing(p);
  }
  return node;
}

static ASTNode *ParseIDList(Parser *p)
{
  ASTNode *node, *id;
  node = MakeNode(listNode, p);
  id = ParseID(p);
  if (IsErrorNode(id)) return ParseFail(node, id);
  NodePush(node, id);
  Spacing(p);
  while (MatchToken(commaToken, p)) {
    VSpacing(p);
    id = ParseID(p);
    if (IsErrorNode(id)) return ParseFail(node, id);
    NodePush(node, id);
    Spacing(p);
  }
  return node;
}

static ASTNode *ParseID(Parser *p)
{
  i32 sym;
  ASTNode *node;
  if (!CheckToken(idToken, p)) return ParseError("Expected identifier", p);
  sym = SymbolFrom(p->text + p->token.pos, p->token.length);
  node = MakeTerminal(idNode, sym, p);
  Adv(p);
  return node;
}

static void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) VSpacing(p);
}
