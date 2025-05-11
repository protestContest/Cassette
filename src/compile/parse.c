#include "compile/parse.h"
#include "compile/node.h"
#include "runtime/mem.h"
#include "runtime/ops.h"
#include "runtime/symbol.h"
#include "univ/hashmap.h"
#include "univ/str.h"

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
} CParseRule;

#define Adv(p) (p)->token = NextToken((p)->text, (p)->token.pos + (p)->token.length)
#define AtEnd(p)            ((p)->token.type == eofToken)
#define CheckToken(t, p)    ((p)->token.type == (t))
#define Lexeme(token, p)    SymbolFrom((p)->text + (token).pos, (token).length)
#define ParseFail(n,r)      ((n) ? FreeNode(n), (r) : (r))
#define Spacing(p)          MatchToken(spaceToken, p)
#define MakeTerminal(type, value, p) \
  NewNode(type, (p)->token.pos, (p)->token.pos + (p)->token.length, value)
#define MakeNode(type, p)   MakeTerminal(type, 0, p)
#define NilNode(p)          NewNode(nilNode, (p)->token.pos, (p)->token.pos, 0)
#define EmptyNode(p)        NewNode(listNode, (p)->token.pos, (p)->token.pos, 0)
#define Expected(s, n, p)   ParseFail(n, ParseError("Expected \"" s "\"", p))
#define ParseExpr(p)        ParsePrec(precExpr, p)

void InitParser(Parser *p, char *text)
{
  p->text = text;
  p->token.pos = 0;
  p->token.length = 0;
  Adv(p);
}

static ASTNode *ParseError(char *msg, Parser *p)
{
  ASTNode *node;
  char *error = NewString("Parse error: ^");
  error = FormatString(error, msg);
  node = ErrorNode(error, p->token.pos, p->token.pos + p->token.length);
  free(error);
  return node;
}

/* Advances if the current token matches a type; returns whether matched */
static bool MatchToken(TokenType type, Parser *p)
{
  if (p->token.type != type) return false;
  Adv(p);
  return true;
}

/* Advances past any spaces or newlines */
static void VSpacing(Parser *p)
{
  Spacing(p);
  if (MatchToken(newlineToken, p)) VSpacing(p);
}

static ASTNode *ParseOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNegOp(ASTNode *expr, Parser *p);
static ASTNode *ParseNotOp(ASTNode *expr, Parser *p);
static ASTNode *ParseDot(ASTNode *expr, Parser *p);
static ASTNode *ParseLogic(ASTNode *expr, Parser *p);
static ASTNode *ParsePair(ASTNode *expr, Parser *p);
static ASTNode *ParseCall(ASTNode *expr, Parser *p);
static ASTNode *ParseAccess(ASTNode *expr, Parser *p);
static ASTNode *ParseID(Parser *p);
static ASTNode *ParseGroup(Parser *p);
static ASTNode *ParseLiteral(Parser *p);
static ASTNode *ParseString(Parser *p);
static ASTNode *ParseSymbol(Parser *p);
static ASTNode *ParseNum(Parser *p);
static ASTNode *ParseHex(Parser *p);
static ASTNode *ParseByte(Parser *p);
static ASTNode *ParseUnary(Parser *p);
static ASTNode *ParseList(Parser *p);
static ASTNode *ParseTuple(Parser *p);
static ASTNode *ParseLambda(Parser *p);
static ASTNode *ParseIf(Parser *p);
static ASTNode *ParseLet(Parser *p);
static ASTNode *ParseGuard(Parser *p);
static ASTNode *ParseRecord(Parser *p);
static ASTNode *ParseDo(Parser *p);

/* Rules indexed by tokenType. */
static CParseRule rules[] = {
  /* eofToken */      {0,             0,              precNone},
  /* newlineToken */  {0,             0,              precNone},
  /* spaceToken */    {0,             0,              precNone},
  /* bangeqToken */   {0,             ParseNotOp,     precEqual},
  /* stringToken */   {ParseString,   0,              precNone},
  /* hashToken */     {ParseUnary,    0,              precNone},
  /* byteToken */     {ParseByte,     0,              precNone},
  /* percentToken */  {0,             ParseOp,        precProduct},
  /* ampToken */      {0,             ParseOp,        precProduct},
  /* lparenToken */   {ParseGroup,    ParseCall,      precCall},
  /* rparenToken */   {0,             0,              precNone},
  /* starToken */     {0,             ParseOp,        precProduct},
  /* plusToken */     {0,             ParseOp,        precSum},
  /* commaToken */    {0,             0,              precNone},
  /* minusToken */    {ParseUnary,    ParseOp,        precSum},
  /* arrowToken */    {0,             0,              precNone},
  /* dotToken */      {0,             ParseDot,       precQualify},
  /* slashToken */    {0,             ParseOp,        precProduct},
  /* numToken */      {ParseNum,      0,              precNone},
  /* hexToken */      {ParseHex,      0,              precNone},
  /* colonToken */    {ParseSymbol,   ParsePair,      precPair},
  /* ltToken */       {0,             ParseOp,        precCompare},
  /* ltltToken */     {0,             ParseOp,        precShift},
  /* lteqToken */     {0,             ParseNotOp,     precCompare},
  /* ltgtToken */     {0,             ParseOp,        precJoin},
  /* eqToken */       {0,             0,              precNone},
  /* eqeqToken */     {0,             ParseOp,        precEqual},
  /* gtToken */       {0,             ParseOp,        precCompare},
  /* gteqToken */     {0,             ParseNotOp,     precCompare},
  /* gtgtToken */     {0,             ParseNegOp,     precShift},
  /* atToken */       {ParseUnary,    0,              precNone},
  /* idToken */       {ParseID,       0,              precNone},
  /* andToken */      {ParseID,       ParseLogic,     precLogic},
  /* asToken */       {ParseID,       0,              precNone},
  /* defToken */      {ParseID,       0,              precNone},
  /* doToken */       {ParseDo,       0,              precNone},
  /* elseToken */     {ParseID,       0,              precNone},
  /* endToken */      {ParseID,       0,              precNone},
  /* falseToken */    {ParseLiteral,  0,              precNone},
  /* guardToken */    {ParseGuard,    0,              precNone},
  /* ifToken */       {ParseIf,       0,              precNone},
  /* importToken */   {ParseID,       0,              precNone},
  /* inToken */       {0,             0,              precNone},
  /* letToken */      {ParseLet,      0,              precNone},
  /* moduleToken */   {ParseID,       0,              precNone},
  /* nilToken */      {ParseLiteral,  0,              precNone},
  /* notToken */      {ParseUnary,    0,              precNone},
  /* orToken */       {ParseID,       ParseLogic,     precLogic},
  /* recordToken */   {ParseRecord,   0,              precNone},
  /* trueToken */     {ParseLiteral,  0,              precNone},
  /* whenToken */     {0,             0,              precNone},
  /* lbraceToken */   {ParseTuple,    0,              precNone},
  /* bslashToken */   {ParseLambda,   0,              precNone},
  /* rbraceToken */   {0,             0,              precNone},
  /* caretToken */    {ParseUnary,    ParseOp,        precSum},
  /* lbracketToken */ {ParseList,     ParseAccess,    precCall},
  /* barToken */      {0,             ParseOp,        precSum},
  /* rbracketToken */ {0,             0,              precNone},
  /* tildeToken */    {ParseUnary,    0,              precNone},
  /* errorToken */    {0,             0,              precNone}
};

/* Parses an expression from the rule table. A matching prefix is parsed, then
 * infixes are parsed as long as the matching precedence is higher. */
static ASTNode *ParsePrec(Precedence prec, Parser *p)
{
  ASTNode *expr;
  CParseRule rule = rules[p->token.type];
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

/* Parses a comma-separated list of expressions. */
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

/* Parses a comma-separated list of IDs. */
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

/* Parses an optional comma separated list of IDs. */
#define ParseParams(p) (CheckToken(idToken, p) ? ParseIDList(p) : EmptyNode(p))

/* Returns the op code an opNode should have. */
static u32 OpCodeFor(TokenType type)
{
  switch (type) {
  case bangeqToken:   return opEq;
  case percentToken:  return opRem;
  case ampToken:      return opAnd;
  case starToken:     return opMul;
  case plusToken:     return opAdd;
  case minusToken:    return opSub;
  case slashToken:    return opDiv;
  case ltToken:       return opLt;
  case ltltToken:     return opShift;
  case lteqToken:     return opGt;
  case ltgtToken:     return opJoin;
  case eqeqToken:     return opEq;
  case gtToken:       return opGt;
  case gteqToken:     return opLt;
  case gtgtToken:     return opShift;
  case caretToken:    return opXor;
  case lbracketToken: return opSlice;
  case barToken:      return opOr;
  case colonToken:    return opPair;
  default:            assert(false);
  }
}

/* Parses an infix op (left-associative). */
static ASTNode *ParseOp(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(opNode, p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  SetNodeAttr(node, "opCode", OpCodeFor(token.type));
  NodePush(node, expr);
  NodePush(node, arg);
  return node;
}

/* Parses an infix op where the second operand should be negated. */
static ASTNode *ParseNegOp(ASTNode *expr, Parser *p)
{
  ASTNode *node = ParseOp(expr, p);
  ASTNode *arg, *neg;
  if (IsErrorNode(node)) return node;

  arg = VecPop(node->data.children);
  neg = MakeNode(opNode, p);
  SetNodeAttr(neg, "opCode", opNeg);
  NodePush(neg, arg);
  NodePush(node, neg);
  return node;
}

/* Parses an infix op where the result should be logically inverted. */
static ASTNode *ParseNotOp(ASTNode *expr, Parser *p)
{
  ASTNode *node = ParseOp(expr, p);
  ASTNode *not;
  if (IsErrorNode(node)) return node;
  not = MakeNode(opNode, p);
  SetNodeAttr(not, "opCode", opNot);
  NodePush(not, node);
  return not;
}

/* Parses an infix reference. */
static ASTNode *ParseDot(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParseID(p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);
  arg->nodeType = symNode;

  node = MakeNode(refNode, p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, arg);
  return node;
}

static ASTNode *ParseLogic(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  NodeType type = (token.type == andToken) ? andNode : orNode;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec + 1, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(type, p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  NodePush(node, expr);
  NodePush(node, arg);
  return node;
}

/* Parses an infix pair op (right associative) */
static ASTNode *ParsePair(ASTNode *expr, Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  VSpacing(p);
  arg = ParsePrec(rules[token.type].prec, p);
  if (IsErrorNode(arg)) return ParseFail(expr, arg);

  node = MakeNode(opNode, p);
  node->start = token.pos;
  node->end = token.pos + token.length;
  SetNodeAttr(node, "opCode", opPair);
  NodePush(node, arg);
  NodePush(node, expr);
  return node;
}

/* Parses a function call. */
static ASTNode *ParseCall(ASTNode *expr, Parser *p)
{
  ASTNode *node, *args;
  u32 start = p->token.pos;
  assert(MatchToken(lparenToken, p));
  node = MakeNode(callNode, p);
  node->start = start;
  NodePush(node, expr);
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) {
    args = ParseItems(p);
    if (IsErrorNode(args)) return ParseFail(node, args);
    NodePush(node, args);
    if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  } else {
    NodePush(node, EmptyNode(p));
  }
  node->end = p->token.pos;
  return node;
}

/* Parses a subscripted access or slice. */
static ASTNode *ParseAccess(ASTNode *expr, Parser *p)
{
  ASTNode *node, *arg;
  Adv(p);
  node = MakeNode(opNode, p);
  node->start = expr->start;
  SetNodeAttr(node, "opCode", opGet);
  NodePush(node, expr);
  VSpacing(p);

  arg = ParseExpr(p);
  if (IsErrorNode(arg)) return ParseFail(node, arg);
  NodePush(node, arg);

  VSpacing(p);
  if (MatchToken(commaToken, p)) {
    SetNodeAttr(node, "opCode", opSlice);
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

/* Parses an identifier. */
static ASTNode *ParseID(Parser *p)
{
  i32 sym;
  ASTNode *node;

  if (p->token.type != idToken && !IsKeyword(p->token.type)) {
    return ParseError("Expected identifier", p);
  }
  sym = SymbolFrom(p->text + p->token.pos, p->token.length);
  node = MakeTerminal(idNode, IntVal(sym), p);
  Adv(p);
  return node;
}

/* Parses a grouped expression. */
static ASTNode *ParseGroup(Parser *p)
{
  ASTNode *node;
  assert(MatchToken(lparenToken, p));
  VSpacing(p);
  node = ParseExpr(p);
  if (IsErrorNode(node)) return node;
  VSpacing(p);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  return node;
}

/* Parses a "nil", "true", or "false" */
static ASTNode *ParseLiteral(Parser *p)
{
  ASTNode *node;
  if (MatchToken(nilToken, p)) {
    node = MakeTerminal(nilNode, 0, p);
  } else if (MatchToken(trueToken, p)) {
    node = MakeTerminal(intNode, IntVal(1), p);
  } else if (MatchToken(falseToken, p)) {
    node = MakeTerminal(intNode, IntVal(0), p);
  } else {
    assert(false);
  }
  return node;
}

static char EscapeChar(char c)
{
  switch (c) {
  case 'a': return '\a';
  case 'b': return '\b';
  case 'e': return 0x1B;
  case 'f': return '\f';
  case 'n': return '\n';
  case 'r': return '\r';
  case 's': return ' ';
  case 't': return '\t';
  case 'v': return '\v';
  case '0': return '\0';
  default:  return c;
  }
}

/* Parses a string. */
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
      ch = EscapeChar(p->text[token.pos + i + 1]);
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

/* Parses a symbol. */
static ASTNode *ParseSymbol(Parser *p)
{
  ASTNode *node;
  i32 sym;
  i32 start = p->token.pos;
  Token token;
  assert(MatchToken(colonToken, p));
  token = p->token;
  sym = SymbolFrom(p->text + token.pos, token.length);
  node = MakeTerminal(symNode, IntVal(sym), p);
  node->start = start;
  node->end = token.pos + token.length;
  Adv(p);
  return node;
}

/* Parses a decimal number. */
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
  node = MakeTerminal(intNode, IntVal(n), p);
  Adv(p);
  return node;
}

/* Parses a hex number. */
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
  node = MakeTerminal(intNode, IntVal(n), p);
  Adv(p);
  return node;
}

/* Parses a byte literal. */
static ASTNode *ParseByte(Parser *p)
{
  ASTNode *node;
  u8 byte;
  char *lexeme = p->text + p->token.pos;
  char ch = (lexeme[1] == '\\') ? lexeme[2] : lexeme[1];
  if (IsSpace(ch) || !IsPrintable(ch)) return ParseError("Expected character", p);
  byte = (lexeme[1] == '\\') ? EscapeChar(ch) : ch;
  node = MakeTerminal(intNode, IntVal(byte), p);
  Adv(p);
  return node;
}

static u32 UnaryOpCodeFor(TokenType type)
{
  switch (type) {
  case minusToken:    return opNeg;
  case hashToken:     return opLen;
  case tildeToken:    return opComp;
  case atToken:       return opHead;
  case caretToken:    return opTail;
  case notToken:      return opNot;
  default:            assert(false);
  }
}

/* Parses a unary op node. */
static ASTNode *ParseUnary(Parser *p)
{
  Token token = p->token;
  ASTNode *arg, *node;
  Adv(p);
  if (token.type == notToken) VSpacing(p);
  arg = ParsePrec(precCall, p);
  if (IsErrorNode(arg)) return arg;
  node = MakeNode(opNode, p);
  node->start = token.pos;
  SetNodeAttr(node, "opCode", UnaryOpCodeFor(token.type));
  NodePush(node, arg);
  return node;
}

/* Parses a list literal. */
static ASTNode *ParseList(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  assert(MatchToken(lbracketToken, p));
  VSpacing(p);
  if (MatchToken(rbracketToken, p)) {
    return NewNode(nilNode, start, p->token.pos, 0);
  }
  node = ParseItems(p);
  if (IsErrorNode(node)) return node;
  if (!MatchToken(rbracketToken, p)) return Expected("]", node, p);
  node->start = start;
  node->end = p->token.pos;
  return node;
}

/* Parses a tuple literal. */
static ASTNode *ParseTuple(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  assert(MatchToken(lbraceToken, p));
  VSpacing(p);
  if (MatchToken(rbraceToken, p)) {
    return NewNode(tupleNode, start, p->token.pos, 0);
  }
  node = ParseItems(p);
  if (IsErrorNode(node)) return node;
  if (!MatchToken(rbraceToken, p)) return Expected("}", node, p);
  node->nodeType = tupleNode;
  node->start = start;
  node->end = p->token.pos;
  return node;
}

/* Parses a lambda. */
static ASTNode *ParseLambda(Parser *p)
{
  ASTNode *params, *body, *node;
  node = MakeNode(lambdaNode, p);
  assert(MatchToken(bslashToken, p));
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

static ASTNode *ParseIf(Parser *p)
{
  assert(MatchToken(ifToken, p));
  VSpacing(p);
  return ParseClauses(p);
}

static ASTNode *ParseAssign(Parser *p)
{
  ASTNode *node = MakeNode(assignNode, p);
  ASTNode *var = ParseID(p);
  ASTNode *value;
  if (IsErrorNode(var)) return ParseFail(node, var);
  NodePush(node, var);
  Spacing(p);
  if (!MatchToken(eqToken, p)) return Expected("=", node, p);
  VSpacing(p);
  value = ParseExpr(p);
  if (IsErrorNode(value)) return ParseFail(node, value);
  NodePush(node, value);
  return node;
}

static ASTNode *ParseAssigns(Parser *p)
{
  ASTNode *node = MakeNode(listNode, p);
  ASTNode *assign;
  do {
    VSpacing(p);
    assign = ParseAssign(p);
    if (IsErrorNode(assign)) return ParseFail(node, assign);
    SetNodeAttr(assign, "index", NodeCount(node));
    NodePush(node, assign);
  } while (MatchToken(commaToken, p));
  return node;
}

static ASTNode *ParseLet(Parser *p)
{
  ASTNode *node = MakeNode(letNode, p);
  ASTNode *assigns;
  assert(MatchToken(letToken, p));
  VSpacing(p);
  assigns = ParseAssigns(p);
  if (IsErrorNode(assigns)) return ParseFail(node, assigns);
  NodePush(node, assigns);
  NodePush(node, NilNode(p));
  return node;
}

static ASTNode *PanicNode(char *msg, Parser *p)
{
  ASTNode *panic = MakeNode(opNode, p);
  ASTNode *msgNode = MakeTerminal(symNode, IntVal(Symbol(msg)), p);
  NodePush(panic, msgNode);
  SetNodeAttr(panic, "opCode", opPanic);
  return panic;
}

static ASTNode *ParseGuard(Parser *p)
{
  ASTNode *node = MakeNode(ifNode, p);
  ASTNode *test, *alt;
  assert(MatchToken(guardToken, p));
  VSpacing(p);
  test = ParseExpr(p);
  if (IsErrorNode(test)) return ParseFail(node, test);
  test = WrapNode(test, opNode);
  SetNodeAttr(test, "opCode", opNot);
  NodePush(node, test);
  VSpacing(p);
  if (!MatchToken(elseToken, p)) return Expected("else", node, p);
  VSpacing(p);
  alt = ParseExpr(p);
  if (IsErrorNode(alt)) return ParseFail(node, alt);
  NodePush(node, alt);
  NodePush(node, NilNode(p));
  SetNodeAttr(node, "guard", 1);
  return node;
}

static ASTNode *RecordBody(ASTNode *params, Parser *p)
{
  u32 i;
  ASTNode *lambda = MakeNode(lambdaNode, p);
  ASTNode *lambdaParams = MakeNode(listNode, p);
  ASTNode *lambdaParam = MakeTerminal(idNode, IntVal(Symbol("field")), p);
  ASTNode *body = MakeNode(ifNode, p);
  NodePush(lambdaParams, lambdaParam);
  NodePush(lambda, lambdaParams);
  NodePush(lambda, body);
  for (i = 0; i < NodeCount(params); i++) {
    ASTNode *param = NodeChild(params, i);
    ASTNode *sym = CloneNode(param);
    ASTNode *predicate = MakeNode(opNode, p);
    SetNodeAttr(predicate, "opCode", opEq);
    NodePush(predicate, CloneNode(lambdaParam));
    sym->nodeType = symNode;
    NodePush(predicate, sym);
    NodePush(body, predicate);
    NodePush(body, CloneNode(param));
    if (i == NodeCount(params) - 1) {
      ASTNode *next = PanicNode("Key not found in record", p);
      NodePush(body, next);
    } else {
      ASTNode *next = MakeNode(ifNode, p);
      NodePush(body, next);
      body = next;
    }
  }
  return lambda;
}

static ASTNode *ParseRecord(Parser *p)
{
  /*
  record Foo(x, y)

  def Foo(x, y)
    \field ->
      if field == :x, x
         field == :y, y
         else panic!("Key not found in record")

  assign
    id Foo
    lambda
      list
        id x
        id y
      lambda
        list
          id field
        if
          eq
            id field
            sym x
          id field
          if
            eq
              id field
              sym y
            id y
            panic
              sym "Key not found in record"
  */
  ASTNode *node = MakeNode(assignNode, p);
  ASTNode *id, *lambda, *params, *body;
  assert(MatchToken(recordToken, p));
  Spacing(p);
  id = ParseID(p);
  if (IsErrorNode(id)) ParseFail(node, id);
  NodePush(node, id);
  lambda = MakeNode(lambdaNode, p);
  NodePush(node, lambda);
  if (!MatchToken(lparenToken, p)) return Expected("(", node, p);
  params = ParseParams(p);
  if (IsErrorNode(params)) return ParseFail(node, params);
  NodePush(lambda, params);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  VSpacing(p);

  body = RecordBody(params, p);
  NodePush(lambda, body);

  return node;
}

static ASTNode *ParseDef(Parser *p)
{
  ASTNode *node = MakeNode(assignNode, p);
  ASTNode *id, *lambda, *params, *body, *guard = 0;
  assert(MatchToken(defToken, p));
  Spacing(p);
  id = ParseID(p);
  if (IsErrorNode(id)) ParseFail(node, id);
  NodePush(node, id);
  lambda = MakeNode(lambdaNode, p);
  NodePush(node, lambda);
  if (!MatchToken(lparenToken, p)) return Expected("(", node, p);
  params = ParseParams(p);
  if (IsErrorNode(params)) return ParseFail(node, params);
  NodePush(lambda, params);
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  VSpacing(p);

  if (MatchToken(whenToken, p)) {
    ASTNode *test;
    Spacing(p);
    test = ParseExpr(p);
    if (IsErrorNode(test)) return ParseFail(node, test);
    Spacing(p);
    if (!(MatchToken(commaToken, p) || MatchToken(newlineToken, p))) {
      FreeNode(test);
      return Expected(",", node, p);
    }
    VSpacing(p);
    guard = MakeNode(ifNode, p);
    SetNodeAttr(guard, "guard", true);
    NodePush(guard, test);
    NodePush(lambda, guard);
  }

  body = ParseExpr(p);
  if (IsErrorNode(body)) return ParseFail(node, body);

  if (guard) {
    NodePush(guard, body);
    NodePush(guard, PanicNode("No matching function", p));
  } else {
    NodePush(lambda, body);
  }

  return node;
}

typedef struct {
  ASTNode ***defs; /* vec of vecs */
  ASTNode **stmts; /* vec */
  HashMap def_map;
} StmtParser;

static void InitStmtParser(StmtParser *sp)
{
  sp->defs = 0;
  sp->stmts = 0;
  InitHashMap(&sp->def_map);
}

static void DestroyStmtParser(StmtParser *sp)
{
  u32 i, j;
  for (i = 0; i < VecCount(sp->defs); i++) {
    for (j = 0; j < VecCount(sp->defs[i]); j++) {
      if (sp->defs[i][j]) FreeNode(sp->defs[i][j]);
    }
    FreeVec(sp->defs[i]);
  }
  FreeVec(sp->defs);
  for (i = 0; i < VecCount(sp->stmts); i++) {
    if (sp->stmts[i]) FreeNode(sp->stmts[i]);
  }
  FreeVec(sp->stmts);
  DestroyHashMap(&sp->def_map);
}

static void StmtParserPushDef(StmtParser *sp, u32 key, ASTNode *node)
{
  u32 index;
  if (HashMapFetch(&sp->def_map, key, &index)) {
    VecPush(sp->defs[index], node);
  } else {
    index = VecCount(sp->defs);
    VecPush(sp->defs, 0);
    VecPush(sp->defs[index], node);
    HashMapSet(&sp->def_map, key, index);
  }
}

static void ReleaseDefs(StmtParser *sp)
{
  FreeVec(sp->defs);
  sp->defs = 0;
  DestroyHashMap(&sp->def_map);
}

static void ReleaseStmts(StmtParser *sp)
{
  FreeVec(sp->stmts);
  sp->stmts = 0;
}

/* Combines a list of defNodes into one. The body of a def's lambdaNode is
 * marked whether its a guard or not. Starting with the body of the first def,
 * as long as the current body is a guard, its alternative is replaced with the
 * body of the next def. This builds an if-else tree of the guard clauses with
 * the function bodies as each consequent. */
static ASTNode *CombineDefs(ASTNode **defs)
{
  u32 i;
  ASTNode *master = defs[0];
  ASTNode *masterParams = NodeChild(NodeChild(master, 1), 0);
  ASTNode *current = NodeChild(NodeChild(master, 1), 1); /* lambda body */
  for (i = 1; i < VecCount(defs); i++) {
    ASTNode *def = defs[i];
    ASTNode *body = NodeChild(NodeChild(def, 1), 1); /* lambda body */
    if (GetNodeAttr(current, "guard")) {
      ASTNode *params = NodeChild(NodeChild(def, 1), 0);
      u32 j;
      if (NodeCount(params) != NodeCount(masterParams)) {
        return ErrorNode("Mismatched parameters", params->start, params->end);
      }
      for (j = 0; j < NodeCount(params); j++) {
        ASTNode *param = NodeChild(params, j);
        if (NodeValue(param) != NodeValue(NodeChild(masterParams, j))) {
          return ErrorNode("Mismatched parameter", param->start, param->end);
        }
      }
      assert(current->nodeType == ifNode);
      /* replace alternative with next def body */
      FreeNode(NodeChild(current, 2));
      NodeChild(current, 2) = body;
      current = body;
    } else {
      FreeNode(def);
    }
  }
  return master;
}

static void AppendDefs(StmtParser *sp, ASTNode *node)
{
  u32 i;
  for (i = 0; i < VecCount(sp->defs); i++) {
    ASTNode *def = CombineDefs(sp->defs[i]);
    SetNodeAttr(def, "index", NodeCount(node));
    NodePush(node, def);
    FreeVec(sp->defs[i]);
  }
  ReleaseDefs(sp);
}

/* Appends a list of stmts into a node. LetNodes and guard nodes capture the
 * remaining statements.
 */
static void AppendStmts(StmtParser *sp, u32 index, ASTNode *node)
{
  u32 i;
  for (i = index; i < VecCount(sp->stmts); i++) {
    ASTNode *stmt = sp->stmts[i];
    NodePush(node, stmt);
    if (stmt->nodeType == letNode) {
      node = NewNode(doNode, stmt->start, node->end, 0);
      SetNodeAttr(node, "numAssigns", 0);
      FreeNode(NodeChild(stmt, 1));
      NodeChild(stmt, 1) = node;
      AppendStmts(sp, i+1, node);
      return;
    } else if (stmt->nodeType == ifNode && NodeHasAttr(stmt, "guard")) {
      node = NewNode(doNode, stmt->start, node->end, 0);
      SetNodeAttr(node, "numAssigns", 0);
      FreeNode(NodeChild(stmt, 2));
      NodeChild(stmt, 2) = node;
      AppendStmts(sp, i+1, node);
      return;
    }
  }
}

/* To parse statements, we separately collect defNodes and other stmt nodes,
 * so we can add the def nodes at the beginning and combine them. */
static ASTNode *ParseStmts(Parser *p)
{
  ASTNode *node;
  StmtParser sp;

  InitStmtParser(&sp);

  /* Parse each statement and save in the statement parser */
  while (!AtEnd(p)) {
    ASTNode *stmt;
    u32 id;
    if (CheckToken(endToken, p)) break;
    if (CheckToken(defToken, p)) {
      stmt = ParseDef(p);
      if (IsErrorNode(stmt)) {
        DestroyStmtParser(&sp);
        return stmt;
      }

      id = NodeValue(NodeChild(stmt, 0));
      StmtParserPushDef(&sp, id, stmt);
    } else if (CheckToken(recordToken, p)) {
      stmt = ParseRecord(p);
      if (IsErrorNode(stmt)) {
        DestroyStmtParser(&sp);
        return stmt;
      }

      id = NodeValue(NodeChild(stmt, 0));
      StmtParserPushDef(&sp, id, stmt);
    } else {
      stmt = ParseExpr(p);
      if (IsErrorNode(stmt)) {
        DestroyStmtParser(&sp);
        return stmt;
      }
      VecPush(sp.stmts, stmt);
    }
    VSpacing(p);
  }
  node = MakeNode(doNode, p);

  /* Add the defNodes, combining each group of the same name into one */
  AppendDefs(&sp, node);
  SetNodeAttr(node, "numAssigns", NodeCount(node));

  /* Add the rest of the statements, combining letNodes as necessary */
  AppendStmts(&sp, 0, node);
  ReleaseStmts(&sp);

  DestroyStmtParser(&sp);

  return node;
}

static ASTNode *ParseDo(Parser *p)
{
  ASTNode *node = 0;
  i32 start = p->token.pos;
  assert(MatchToken(doToken, p));
  VSpacing(p);
  node = ParseStmts(p);
  if (IsErrorNode(node)) return node;
  node->start = start;
  if (!MatchToken(endToken, p)) return Expected("end", node, p);
  node->end = p->token.pos;
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

static ASTNode *ParseAlias(ASTNode *name, Parser *p)
{
  if (!MatchToken(asToken, p)) return CloneNode(name);
  Spacing(p);
  return ParseID(p);
}

static ASTNode *ParseFnList(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(lparenToken, p)) return EmptyNode(p);
  node = ParseIDList(p);
  if (IsErrorNode(node)) return node;
  if (!MatchToken(rparenToken, p)) return Expected(")", node, p);
  node->start = start;
  node->end =  p->token.pos;
  return node;
}

static ASTNode *ParseImport(Parser *p)
{
  ASTNode *node, *name, *alias, *fns;
  node = MakeNode(importNode, p);

  name = ParseID(p);
  if (IsErrorNode(name)) return ParseFail(node, name);
  NodePush(node, name);
  Spacing(p);
  alias = ParseAlias(name, p);
  if (IsErrorNode(alias)) return ParseFail(node, alias);
  NodePush(node, alias);
  Spacing(p);
  fns = ParseFnList(p);
  if (IsErrorNode(fns)) return ParseFail(node, fns);
  NodePush(node, fns);
  node->end = p->token.pos;
  return node;
}

ASTNode *ParseImportList(Parser *p)
{
  ASTNode *node, *import;
  node = MakeNode(tupleNode, p);
  do {
    VSpacing(p);
    import = ParseImport(p);
    if (IsErrorNode(import)) return ParseFail(node, import);
    NodePush(node, import);
  } while (MatchToken(commaToken, p));
  node->end = p->token.pos;
  return node;
}

static ASTNode *ParseImports(Parser *p)
{
  ASTNode *node;
  u32 start = p->token.pos;
  if (!MatchToken(importToken, p)) return EmptyNode(p);
  VSpacing(p);
  node = ParseImportList(p);
  if (IsErrorNode(node)) return node;
  node->end = p->token.pos;
  if (!MatchToken(newlineToken, p)) return Expected("newline", node, p);
  node->start = start;
  node->end = p->token.pos;
  VSpacing(p);
  return node;
}

static ASTNode *FindExports(ASTNode *body, Parser *p)
{
  ASTNode *exports;
  u32 i, count;
  if (body->nodeType != doNode) return EmptyNode(p);

  count = GetNodeAttr(body, "numAssigns");
  exports = MakeNode(tupleNode, p);

  /* depends on exported defs being first in body */
  for (i = 0; i < count; i++) {
    ASTNode *child = NodeChild(body, i);
    assert(child->nodeType == assignNode);
    NodePush(exports, CloneNode(NodeChild(child, 0)));
  }
  return exports;
}

ASTNode *ParseModuleHeader(char *source)
{
  Parser p;
  ASTNode *node, *name, *imports;
  InitParser(&p, source);

  VSpacing(&p);
  node = MakeNode(moduleNode, &p);

  name = ParseModuleName(&p);
  if (IsErrorNode(name)) return ParseFail(node, name);
  NodePush(node, name);

  imports = ParseImports(&p);
  if (IsErrorNode(imports)) return ParseFail(node, imports);
  NodePush(node, imports);

  return node;
}

ASTNode *ParseModule(char *source)
{
  Parser p;
  ASTNode *node, *name, *imports, *exports, *body;
  InitParser(&p, source);

  VSpacing(&p);
  node = MakeNode(moduleNode, &p);

  name = ParseModuleName(&p);
  if (IsErrorNode(name)) return ParseFail(node, name);
  NodePush(node, name);

  imports = ParseImports(&p);
  if (IsErrorNode(imports)) return ParseFail(node, imports);
  NodePush(node, imports);

  VSpacing(&p);
  body = ParseStmts(&p);
  if (IsErrorNode(body)) return ParseFail(node, body);

  if (name->nodeType == idNode) {
    VSpacing(&p);
    exports = FindExports(body, &p);
    if (IsErrorNode(exports)) return ParseFail(node, exports);
    NodePush(node, exports);
    NodePush(body, CloneNode(exports));
  } else {
    NodePush(node, EmptyNode(&p));
  }

  NodePush(node, body);

  if (!AtEnd(&p)) return Expected("End of file", node, &p);
  node->end = p.token.pos;

  return node;
}
