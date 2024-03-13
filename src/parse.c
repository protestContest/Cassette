#include "parse.h"
#include "node.h"
#include <univ.h>

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define ParseError(msg, p)  Error(msg, (p)->filename, TokenPos(&(p)->lex))
#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
#define ParseExpr(p)        ParsePrec(PrecLambda, p)

typedef struct {
  char *filename;
  Lexer lex;
} Parser;

typedef enum {
  PrecNone,
  PrecLambda,
  PrecLogic,
  PrecEqual,
  PrecPair,
  PrecJoin,
  PrecSplit,
  PrecCompare,
  PrecShift,
  PrecSum,
  PrecProduct,
  PrecUnary,
  PrecCall,
  PrecAccess
} Precedence;

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Node *prefix, Parser *p);

static Result ParseModule(Parser *p);
static Result ParseImport(Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseAssign(Parser *p);
static Result ParseDef(Parser *p, u32 pos);
static Result ParsePrec(Precedence prec, Parser *p);
static Result ParseLambda(Parser *p);
static Result ParseLeftAssoc(Node *prefix, Parser *p);
static Result ParseRightAssoc(Node *prefix, Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseCall(Node *prefix, Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseClause(Parser *p);
static Result ParseList(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseLiteral(Parser *p);
static Result ParseID(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseString(Parser *p);

typedef struct {
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[] = {
  /* TokenEOF */        {0,             0,                PrecNone},
  /* TokenNewline */    {0,             0,                PrecNone},
  /* TokenBangEq */     {0,             0,                PrecNone},
  /* TokenStr */        {ParseString,   0,                PrecNone},
  /* TokenHash */       {ParseUnary,    0,                PrecNone},
  /* TokenPercent */    {0,             ParseLeftAssoc,   PrecProduct},
  /* TokenAmpersand */  {0,             ParseLeftAssoc,   PrecProduct},
  /* TokenLParen */     {ParseGroup,    ParseCall,        PrecCall},
  /* TokenRParen */     {0,             0,                PrecNone},
  /* TokenStar */       {0,             ParseLeftAssoc,   PrecProduct},
  /* TokenPlus */       {0,             ParseLeftAssoc,   PrecSum},
  /* TokenComma */      {0,             0,                PrecNone},
  /* TokenMinus */      {ParseUnary,    ParseLeftAssoc,   PrecSum},
  /* TokenArrow */      {0,             0,                PrecNone},
  /* TokenDot */        {0,             ParseLeftAssoc,   PrecAccess},
  /* TokenSlash */      {0,             ParseLeftAssoc,   PrecProduct},
  /* TokenNum */        {ParseNum,      0,                PrecNone},
  /* TokenColon */      {0,             ParseLeftAssoc,   PrecSplit},
  /* TokenLt */         {0,             ParseLeftAssoc,   PrecCompare},
  /* TokenLtLt */       {0,             ParseLeftAssoc,   PrecShift},
  /* TokenLtGt */       {0,             ParseLeftAssoc,   PrecJoin},
  /* TokenEq */         {0,             0,                PrecNone},
  /* TokenEqEq */       {0,             ParseLeftAssoc,   PrecEqual},
  /* TokenGt */         {0,             ParseLeftAssoc,   PrecCompare},
  /* TokenGtGt */       {0,             ParseLeftAssoc,   PrecShift},
  /* TokenAt */         {0,             ParseLeftAssoc,   PrecSplit},
  /* TokenLBracket */   {ParseList,     0,                PrecNone},
  /* TokenBackslash */  {ParseLambda,   0,                PrecNone},
  /* TokenRBracket */   {0,             0,                PrecNone},
  /* TokenCaret */      {0,             ParseLeftAssoc,   PrecSum},
  /* TokenAnd */        {0,             ParseLeftAssoc,   PrecLogic},
  /* TokenAs */         {0,             0,                PrecNone},
  /* TokenCond */       {ParseCond,     0,                PrecNone},
  /* TokenDef */        {0,             0,                PrecNone},
  /* TokenDo */         {ParseDo,       0,                PrecNone},
  /* TokenElse */       {0,             0,                PrecNone},
  /* TokenEnd */        {0,             0,                PrecNone},
  /* TokenFalse */      {ParseLiteral,  0,                PrecNone},
  /* TokenIf */         {ParseIf,       0,                PrecNone},
  /* TokenImport */     {0,             0,                PrecNone},
  /* TokenLet */        {0,             0,                PrecNone},
  /* TokenModule */     {0,             0,                PrecNone},
  /* TokenNil */        {ParseLiteral,  0,                PrecNone},
  /* TokenNot */        {ParseUnary,    0,                PrecNone},
  /* TokenOr */         {0,             ParseLeftAssoc,   PrecLogic},
  /* TokenTrue */       {ParseLiteral,  0,                PrecNone},
  /* TokenLBrace */     {ParseTuple,    0,                PrecNone},
  /* TokenBar */        {0,             ParseRightAssoc,  PrecPair},
  /* TokenRBrace */     {0,             0,                PrecNone},
  /* TokenTilde */      {ParseUnary,    0,                PrecNone},
  /* TokenID */         {ParseID,       0,                PrecNone}
};

static NodeType UnaryOpNode(TokenType token_type);
static NodeType BinaryOpNode(TokenType token_type);

static Result ParseFail(Result result, void *node);

Result ParseFile(char *filename)
{
  Parser p;
  Result result;
  char *source = ReadFile(filename);
  if (source == 0) return Error("Could not read file", filename, 0);
  p.filename = filename;
  InitLexer(&p.lex, source, 0);
  SkipNewlines(&p.lex);
  result = ParseModule(&p);
  free(source);
  return result;
}

Result Parse(char *source)
{
  Parser p;
  InitLexer(&p.lex, source, 0);
  SkipNewlines(&p.lex);
  return ParseExpr(&p);
}

static Result ParseModule(Parser *p)
{
  Result result;
  Node *module = MakeNode(moduleNode, 0);
  Node *imports = MakeNode(listNode, 0);
  u32 main = AddSymbol("*main*");

  /* module name */
  if (MatchToken(TokenModule, &p->lex)) {
    result = ParseID(p);
    if (!IsOk(result)) return ParseFail(result, module);
    NodePush(module, result.value);
    free(result.value);
  } else {
    NodePush(module, MakeTerminal(idNode, 0, main));
  }

  /* imports */
  SkipNewlines(&p->lex);
  imports->pos = TokenPos(&p->lex);
  NodePush(module, imports);
  while (MatchToken(TokenImport, &p->lex)) {
    result = ParseImport(p);
    if (!IsOk(result)) return ParseFail(result, module);
    NodePush(imports, result.value);
  }

  /* body */
  result = ParseStmts(p);
  if (!IsOk(result)) return ParseFail(result, module);
  if (!MatchToken(TokenEOF, &p->lex)) {
    return ParseFail(ParseError("Unexpected token", p), module);
  }
  NodePush(module, result.value);

  NodePush(module, MakeTerminal(idNode, 0, AddSymbol(p->filename)));

  return Ok(module);
}

static Result ParseImport(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Node *import_node = MakeNode(importNode, pos);

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, import_node);
  NodePush(import_node, result.value);

  if (MatchToken(TokenAs, &p->lex)) {
    result = ParseID(p);
    if (!IsOk(result)) return ParseFail(result, import_node);
    NodePush(import_node, result.value);
  } else {
    Node *mod = result.value;
    NodePush(import_node, MakeTerminal(idNode, mod->pos, mod->value));
  }

  SkipNewlines(&p->lex);

  return Ok(import_node);
}

static Result ParseStmts(Parser *p)
{
  Result result;
  Node *node = MakeNode(doNode, TokenPos(&p->lex));
  Node *locals = MakeNode(listNode, TokenPos(&p->lex));
  Node *stmts = MakeNode(listNode, TokenPos(&p->lex));
  NodePush(node, locals);
  NodePush(node, stmts);

  SkipNewlines(&p->lex);
  while (!EndOfBlock(LexPeek(&p->lex))) {
    u32 pos = TokenPos(&p->lex);

    if (MatchToken(TokenLet, &p->lex)) {
      SkipNewlines(&p->lex);
      while (!MatchToken(TokenNewline, &p->lex)) {
        Node *assign;
        if (MatchToken(TokenEOF, &p->lex)) break;

        result = ParseAssign(p);
        if (!IsOk(result)) return ParseFail(result, node);
        assign = result.value;
        NodePush(stmts, assign);
        NodePush(locals, AssignNodeVar(assign));

        if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
      }
    } else if (MatchToken(TokenDef, &p->lex)) {
      Node *assign;
      result = ParseDef(p, pos);
      if (!IsOk(result)) return ParseFail(result, node);
      assign = result.value;
      NodePush(stmts, assign);
      NodePush(locals, AssignNodeVar(assign));
    } else {
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      NodePush(stmts, result.value);
    }

    SkipNewlines(&p->lex);
  }

  if (NodeCount(stmts) == 0) {
    NodePush(stmts, MakeTerminal(nilNode, TokenPos(&p->lex), 0));
  }
  if (NodeCount(locals) == 0 && NodeCount(stmts) == 1) {
    Node *stmt = stmts->children[0];
    FreeNode(locals);
    FreeVec(stmts->children);
    free(node);
    return Ok(stmt);
  }

  return Ok(node);
}

static Result ParseAssign(Parser *p)
{
  Result result;
  Node *node = MakeNode(letNode, TokenPos(&p->lex));

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) {
    return ParseFail(ParseError("Expected \"=\"", p), node);
  }
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  return Ok(node);
}

static Result ParseDef(Parser *p, u32 pos)
{
  Result result;
  Node *node = MakeNode(defNode, pos);
  Node *lambda, *params;

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  lambda = MakeNode(lambdaNode, TokenPos(&p->lex));
  NodePush(node, lambda);

  if (!MatchToken(TokenLParen, &p->lex)) return ParseFail(result, node);
  params = MakeNode(listNode, TokenPos(&p->lex));
  NodePush(lambda, params);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!IsOk(result)) return ParseFail(result, node);
      NodePush(params, result.value);
    } while (MatchToken(TokenComma, &p->lex));
    SkipNewlines(&p->lex);
    if (!MatchToken(TokenRParen, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
    }
  }
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(lambda, result.value);

  return Ok(node);
}

static Result ParsePrec(Precedence prec, Parser *p)
{
  Result result;

  if (!ExprNext(&p->lex)) return ParseError("Expected expression", p);
  result = ExprNext(&p->lex)(p);

  while (IsOk(result) && PrecNext(&p->lex) >= prec) {
    result = rules[p->lex.token.type].infix(result.value, p);
  }

  return result;
}

static Result ParseUnary(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Node *node = MakeNode(UnaryOpNode(token.type), pos);

  SkipNewlines(&p->lex);
  result = ParsePrec(PrecUnary, p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  return Ok(node);
}

static Result ParseLeftAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(BinaryOpNode(token.type), pos);

  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  return Ok(node);
}

static Result ParseRightAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Node *node = MakeNode(BinaryOpNode(token.type), pos);
  NodePush(node, prefix);

  SkipNewlines(&p->lex);
  result = ParsePrec(prec, p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, prefix);
  node->children[0] = result.value;

  return Ok(node);
}

static Result ParseCall(Node *prefix, Parser *p)
{
  Result result;
  Node *node = MakeNode(callNode, TokenPos(&p->lex));
  NodePush(node, prefix);

  MatchToken(TokenLParen, &p->lex);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      NodePush(node, result.value);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRParen, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
    }
  }

  return Ok(node);
}

static Result ParseLambda(Parser *p)
{
  Result result;
  Node *node = MakeNode(lambdaNode, TokenPos(&p->lex));
  Node *params;

  if (!MatchToken(TokenBackslash, &p->lex)) {
    return ParseFail(ParseError("Expected \"\\\"", p), node);
  }

  params = MakeNode(listNode, TokenPos(&p->lex));
  NodePush(node, params);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenArrow, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!IsOk(result)) return ParseFail(result, node);
      NodePush(params, result.value);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \"->\"", p), node);
    }
  }

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  return Ok(node);
}

static Result ParseGroup(Parser *p)
{
  Result result;
  assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!IsOk(result)) return result;
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    return ParseFail(ParseError("Expected \")\"", p), result.value);
  }
  return result;
}

static Result ParseDo(Parser *p)
{
  Result result;
  Node *node;
  u32 pos = TokenPos(&p->lex);
  assert(MatchToken(TokenDo, &p->lex));

  result = ParseStmts(p);
  if (!IsOk(result)) return result;
  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  node = result.value;
  node->pos = pos;

  return result;
}

static Result ParseIf(Parser *p)
{
  Result result;
  Node *node = MakeNode(ifNode, TokenPos(&p->lex));

  assert(MatchToken(TokenIf, &p->lex));

  /* predicate */
  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  if (!MatchToken(TokenDo, &p->lex)) {
    return ParseFail(ParseError("Expected \"do\"", p), node);
  }

  /* consequent */
  result = ParseStmts(p);
  if (!IsOk(result)) return ParseFail(result, node);
  NodePush(node, result.value);

  /* alternative */
  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!IsOk(result)) return ParseFail(result, node);
    NodePush(node, result.value);
  } else {
    NodePush(node, MakeTerminal(nilNode, TokenPos(&p->lex), 0));
  }

  if (!MatchToken(TokenEnd, &p->lex)) {
    return ParseFail(ParseError("Expected \"end\"", p), node);
  }

  return Ok(node);
}

static Result ParseClauses(Parser *p)
{
  if (MatchToken(TokenEnd, &p->lex)) {
    return Ok(MakeTerminal(nilNode, TokenPos(&p->lex), 0));
  } else {
    Result result;
    Node *node = MakeNode(ifNode, TokenPos(&p->lex));

    result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    NodePush(node, result.value);

    SkipNewlines(&p->lex);
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \"->\"", p), node);
    }
    SkipNewlines(&p->lex);

    result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    NodePush(node, result.value);

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!IsOk(result)) return ParseFail(result, node);
    NodePush(node, result.value);

    return Ok(node);
  }
}

static Result ParseCond(Parser *p)
{
  assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Result ParseList(Parser *p)
{
  Result result;
  Node *node = MakeNode(listNode, TokenPos(&p->lex));

  assert(MatchToken(TokenLBracket, &p->lex));
  SkipNewlines(&p->lex);

  if (!MatchToken(TokenRBracket, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      NodePush(node, result.value);
      SkipNewlines(&p->lex);
    } while(MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRBracket, &p->lex)) {
      return ParseError("Expected \")\"", p);
    }
  }

  return Ok(node);
}

static Result ParseTuple(Parser *p)
{
  Node *node = MakeNode(tupleNode, TokenPos(&p->lex));
  assert(MatchToken(TokenLBrace, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    NodePush(node, result.value);

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return Ok(node);
}

static Result ParseID(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  u32 name;
  if (token.type != TokenID) return ParseError("Expected identifier", p);

  name = AddSymbolLen(token.lexeme, token.length);
  return Ok(MakeTerminal(idNode, pos, name));
}

static Result ParseNum(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  u32 whole = 0, frac = 0, frac_size = 1, i;
  i32 sign = 1;

  while (*token.lexeme == '0' || *token.lexeme == '_') {
    token.lexeme++;
    token.length--;
  }

  for (i = 0; i < token.length; i++) {
    u32 digit;
    if (token.lexeme[i] == '_') continue;
    if (token.lexeme[i] == '.') {
      i++;
      break;
    }
    digit = token.lexeme[i] - '0';

    if (whole > MaxUInt/10) return ParseError("Number overflows", p);
    whole *= 10;

    if (whole > MaxUInt - digit) return ParseError("Number overflows", p);
    whole += digit;
  }

  for (; i < token.length; i++) {
    if (token.lexeme[i] == '_') continue;
    frac_size *= 10;
    frac = frac * 10 + token.lexeme[i] - '0';
  }

  if (frac != 0) {
    float num = (float)whole + (float)frac / (float)frac_size;
    return Ok(MakeTerminal(floatNode, pos, sign*num));
  } else {
    return Ok(MakeTerminal(intNode, pos, sign*whole));
  }
}

static Result ParseString(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  u32 name = AddSymbolLen(token.lexeme + 1, token.length - 2);
  return Ok(MakeTerminal(stringNode, pos, name));
}

static Result ParseLiteral(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);

  switch (token.type) {
  case TokenTrue:   return Ok(MakeTerminal(intNode, pos, 1));
  case TokenFalse:  return Ok(MakeTerminal(intNode, pos, 0));
  case TokenNil:    return Ok(MakeTerminal(intNode, pos, 0));
  default:          return ParseError("Unknown literal", p);
  }
}

/* Helper to deallocate a node and return a result */
static Result ParseFail(Result result, void *node)
{
  FreeNode(node);
  return result;
}

/* Helpers to map operator tokens to symbols */
static NodeType UnaryOpNode(TokenType token_type)
{
  switch (token_type) {
  case TokenHash:       return lenNode;
  case TokenMinus:      return negNode;
  case TokenNot:        return notNode;
  case TokenTilde:      return bitNotNode;
  default:              return nilNode;
  }
}

static NodeType BinaryOpNode(TokenType token_type)
{
  switch (token_type) {
  case TokenPercent:    return remNode;
  case TokenAmpersand:  return bitAndNode;
  case TokenStar:       return mulNode;
  case TokenPlus:       return addNode;
  case TokenMinus:      return subNode;
  case TokenDot:        return accessNode;
  case TokenSlash:      return divNode;
  case TokenColon:      return splitNode;
  case TokenLt:         return ltNode;
  case TokenLtLt:       return shiftNode;
  case TokenLtGt:       return joinNode;
  case TokenEqEq:       return eqNode;
  case TokenGt:         return gtNode;
  case TokenGtGt:       return shiftNode;
  case TokenAt:         return tailNode;
  case TokenCaret:      return bitOrNode;
  case TokenBar:        return pairNode;
  case TokenAnd:        return andNode;
  case TokenOr:         return orNode;
  default:              return nilNode;
  }
}
