#include "parse.h"
#include "node.h"
#include <univ.h>

typedef struct {
  char *filename;
  Lexer lex;
} Parser;

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define ParseError(msg, p)  Error(msg, (p)->filename, TokenPos(&(p)->lex))
#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
#define ParseExpr(p)        ParsePrec(PrecExpr, p)
#define IDValue(res)        (((TerminalNode*)(res).value)->value)

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLambda,
  PrecLogic,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecBitwise,
  PrecShift,
  PrecSum,
  PrecProduct,
  PrecUnary,
  PrecCall,
  PrecAccess
} Precedence;

static Result ParseModule(Parser *p);
static Node *WithExport(Node *node, Parser *p);
static Result ParseImport(Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseAssign(Parser *p);
static Result ParseDef(Parser *p, u32 pos);
static Result ParsePrec(Precedence prec, Parser *p);
static Result ParseLeftAssoc(Node *prefix, Parser *p);
static Result ParseRightAssoc(Node *prefix, Parser *p);
static Result ParseCall(Node *prefix, Parser *p);
static Result ParseAccess(Node *prefix, Parser *p);
static Result ParseLambda(Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseClause(Parser *p);
static Result ParseList(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseLiteral(Parser *p);
static Result ParseSymbol(Parser *p);
static Result ParseVar(Parser *p);
static Result ParseID(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseString(Parser *p);

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
  ModuleNode *module = (ModuleNode*)MakeNode(moduleNode, 0);
  u32 main = AddSymbol("*main*");

  module->filename = AddSymbol(p->filename);

  /* module name */
  if (MatchToken(TokenModule, &p->lex)) {
    result = ParseID(p);
    if (!IsOk(result)) return ParseFail(result, module);
    module->name = IDValue(result);
    free(result.value);
  } else {
    module->name = main;
  }

  /* imports */
  SkipNewlines(&p->lex);
  while (MatchToken(TokenImport, &p->lex)) {
    result = ParseImport(p);
    if (!IsOk(result)) return ParseFail(result, module);
    VecPush(module->imports, result.value);
  }

  /* body */
  result = ParseStmts(p);
  if (!IsOk(result)) return ParseFail(result, module);
  if (!MatchToken(TokenEOF, &p->lex)) {
    return ParseFail(ParseError("Unexpected token", p), module);
  }
  module->body = result.value;

  return Ok(module);
}

static Result ParseImport(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  ImportNode *import_node = (ImportNode*)MakeNode(importNode, pos);

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, import_node);
  import_node->mod = IDValue(result);
  free(result.value);

  if (MatchToken(TokenAs, &p->lex)) {
    result = ParseID(p);
    if (!IsOk(result)) return ParseFail(result, import_node);
    import_node->alias = IDValue(result);
  } else {
    import_node->alias = import_node->mod;
  }

  SkipNewlines(&p->lex);

  return Ok(import_node);
}

static Result ParseStmts(Parser *p)
{
  Result result;
  DoNode *node = (DoNode*)MakeNode(doNode, TokenPos(&p->lex));

  SkipNewlines(&p->lex);
  while (!EndOfBlock(LexPeek(&p->lex))) {
    u32 pos = TokenPos(&p->lex);

    if (MatchToken(TokenLet, &p->lex)) {
      SkipNewlines(&p->lex);
      while (!MatchToken(TokenNewline, &p->lex)) {
        LetNode *let;
        if (MatchToken(TokenEOF, &p->lex)) break;

        result = ParseAssign(p);
        if (!IsOk(result)) return ParseFail(result, node);
        let = result.value;
        VecPush(node->stmts, (Node*)let);
        VecPush(node->locals, let->var);

        if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
      }
    } else if (MatchToken(TokenDef, &p->lex)) {
      LetNode *let;
      result = ParseDef(p, pos);
      if (!IsOk(result)) return ParseFail(result, node);
      let = result.value;
      VecPush(node->stmts, (Node*)let);
      VecPush(node->locals, let->var);
    } else {
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      VecPush(node->stmts, result.value);
    }

    SkipNewlines(&p->lex);
  }

  if (VecCount(node->stmts) == 0) {
    VecPush(node->stmts, (Node*)MakeTerminal(nilNode, TokenPos(&p->lex), 0));
  }

  return Ok(node);
}

static Result ParseAssign(Parser *p)
{
  Result result;
  LetNode *node = (LetNode*)MakeNode(letNode, TokenPos(&p->lex));

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->var = IDValue(result);
  free(result.value);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) {
    return ParseFail(ParseError("Expected \"=\"", p), node);
  }
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->value = result.value;

  return Ok(node);
}

static Result ParseDef(Parser *p, u32 pos)
{
  Result result;
  LetNode *node = (LetNode*)MakeNode(defNode, pos);
  LambdaNode *lambda;

  result = ParseID(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->var = IDValue(result);
  free(result.value);

  lambda = (LambdaNode*)MakeNode(lambdaNode, pos);
  node->value = (Node*)lambda;

  if (MatchToken(TokenLParen, &p->lex)) {
    SkipNewlines(&p->lex);
    if (!MatchToken(TokenRParen, &p->lex)) {
      do {
        SkipNewlines(&p->lex);
        result = ParseID(p);
        if (!IsOk(result)) return ParseFail(result, node);
        VecPush(lambda->params, IDValue(result));
        free(result.value);
      } while (MatchToken(TokenComma, &p->lex));
      SkipNewlines(&p->lex);
      if (!MatchToken(TokenRParen, &p->lex)) {
        return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
      }
    }
    SkipNewlines(&p->lex);
  }

  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  lambda->body = result.value;

  return Ok(node);
}

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Node *prefix, Parser *p);

typedef struct {
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[] = {
  /* TokenEOF */        {0,            0,                 PrecNone},
  /* TokenNewline */    {0,            0,                 PrecNone},
  /* TokenBangEq */     {0,            ParseLeftAssoc,    PrecEqual},
  /* TokenStr */        {ParseString,  0,                 PrecNone},
  /* TokenHash */       {ParseUnary,   0,                 PrecNone},
  /* TokenPercent */    {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenAmpersand */  {0,            ParseLeftAssoc,    PrecBitwise},
  /* TokenLParen */     {ParseGroup,   ParseCall,         PrecCall},
  /* TokenRParen */     {0,            0,                 PrecNone},
  /* TokenStar */       {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenPlus */       {0,            ParseLeftAssoc,    PrecSum},
  /* TokenComma */      {0,            0,                 PrecNone},
  /* TokenMinus */      {ParseUnary,   ParseLeftAssoc,    PrecSum},
  /* TokenArrow */      {0,            0,                 PrecNone},
  /* TokenDot */        {0,            ParseAccess,       PrecAccess},
  /* TokenDotDot */     {0,            ParseLeftAssoc,    PrecAccess},
  /* TokenSlash */      {0,            ParseLeftAssoc,    PrecProduct},
  /* TokenNum */        {ParseNum,     0,                 PrecNone},
  /* TokenColon */      {ParseSymbol,  0,                 PrecNone},
  /* TokenLt */         {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenLtLt */       {0,            ParseLeftAssoc,    PrecShift},
  /* TokenLtEq */       {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenLtGt */       {0,            ParseLeftAssoc,    PrecPair},
  /* TokenEq */         {0,            0,                 PrecNone},
  /* TokenEqEq */       {0,            ParseLeftAssoc,    PrecEqual},
  /* TokenGt */         {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenGtEq */       {0,            ParseLeftAssoc,    PrecCompare},
  /* TokenGtGt */       {0,            ParseLeftAssoc,    PrecShift},
  /* TokenLBracket */   {ParseList,    0,                 PrecNone},
  /* TokenBackslash */  {ParseLambda,  0,                 PrecNone},
  /* TokenRBracket */   {0,            0,                 PrecNone},
  /* TokenCaret */      {0,            ParseLeftAssoc,    PrecBitwise},
  /* TokenAnd */        {0,            ParseLeftAssoc,    PrecLogic},
  /* TokenAs */         {0,            0,                 PrecNone},
  /* TokenCond */       {ParseCond,    0,                 PrecNone},
  /* TokenDef */        {0,            0,                 PrecNone},
  /* TokenDo */         {ParseDo,      0,                 PrecNone},
  /* TokenElse */       {0,            0,                 PrecNone},
  /* TokenEnd */        {0,            0,                 PrecNone},
  /* TokenFalse */      {ParseLiteral, 0,                 PrecNone},
  /* TokenIf */         {ParseIf,      0,                 PrecNone},
  /* TokenImport */     {0,            0,                 PrecNone},
  /* TokenIn */         {0,            ParseLeftAssoc,    PrecMember},
  /* TokenLet */        {0,            0,                 PrecNone},
  /* TokenModule */     {0,            0,                 PrecNone},
  /* TokenNil */        {ParseLiteral, 0,                 PrecNone},
  /* TokenNot */        {ParseUnary,   0,                 PrecNone},
  /* TokenOr */         {0,            ParseLeftAssoc,    PrecLogic},
  /* TokenTrue */       {ParseLiteral, 0,                 PrecNone},
  /* TokenLBrace */     {ParseTuple,   0,                 PrecNone},
  /* TokenBar */        {0,            ParseRightAssoc,   PrecPair},
  /* TokenRBrace */     {0,            0,                 PrecNone},
  /* TokenTilde */      {ParseUnary,   0,                 PrecNone},
  /* TokenID */         {ParseVar,     0,                 PrecNone}
};

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
  UnaryNode *node = (UnaryNode*)MakeNode(UnaryOpNode(token.type), pos);

  SkipNewlines(&p->lex);
  result = ParsePrec(PrecUnary, p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->child = result.value;

  return Ok(node);
}

static Result ParseLeftAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  BinaryNode *node = (BinaryNode*)MakeNode(BinaryOpNode(token.type), pos);
  node->left = prefix;

  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->right = result.value;

  return Ok(node);
}

static Result ParseRightAssoc(Node *prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  BinaryNode *node = (BinaryNode*)MakeNode(BinaryOpNode(token.type), pos);
  node->right = prefix;

  SkipNewlines(&p->lex);
  result = ParsePrec(prec, p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->left = result.value;

  return Ok(node);
}

static Result ParseCall(Node *prefix, Parser *p)
{
  Result result;
  CallNode *node = (CallNode*)MakeNode(callNode, TokenPos(&p->lex));
  node->op = prefix;

  MatchToken(TokenLParen, &p->lex);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      VecPush(node->args, result.value);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRParen, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \")\"", p), node);
    }
  }

  return Ok(node);
}

static Result ParseAccess(Node *prefix, Parser *p)
{
  Result result;
  BinaryNode *node = (BinaryNode*)MakeNode(accessNode, TokenPos(&p->lex));
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  node->left = prefix;

  if (LexPeek(&p->lex) == TokenID) {
    u32 pos = TokenPos(&p->lex);
    result = ParseID(p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->right = (Node*)MakeTerminal(symbolNode, pos, IDValue(result));
    free(result.value);
  } else {
    result = ParsePrec(prec+1, p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->right = result.value;
  }

  return Ok(node);
}

static Result ParseLambda(Parser *p)
{
  LambdaNode *node = (LambdaNode*)MakeNode(lambdaNode, TokenPos(&p->lex));
  Result result;

  if (!MatchToken(TokenBackslash, &p->lex)) {
    return ParseFail(ParseError("Expected \"\\\"", p), node);
  }

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenArrow, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!IsOk(result)) return ParseFail(result, node);
      VecPush(node->params, IDValue(result));
      free(result.value);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \",\" or \"->\"", p), node);
    }
  }

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->body = result.value;

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
  DoNode *node;
  u32 pos = TokenPos(&p->lex);
  assert(MatchToken(TokenDo, &p->lex));

  result = ParseStmts(p);
  if (!IsOk(result)) return result;
  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  node = result.value;
  node->pos = pos;

  if (VecCount(node->locals) == 0 && VecCount(node->stmts) == 1) {
    Node *stmt = node->stmts[0];
    free(node);
    return Ok(stmt);
  }

  return result;
}

static Result ParseIf(Parser *p)
{
  Result result;
  IfNode *node = (IfNode*)MakeNode(ifNode, TokenPos(&p->lex));

  assert(MatchToken(TokenIf, &p->lex));

  /* predicate */
  result = ParseExpr(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->predicate = result.value;

  if (!MatchToken(TokenDo, &p->lex)) {
    return ParseFail(ParseError("Expected \"do\"", p), node);
  }

  /* consequent */
  result = ParseStmts(p);
  if (!IsOk(result)) return ParseFail(result, node);
  node->ifTrue = result.value;

  /* alternative */
  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->ifFalse = result.value;
  } else {
    node->ifFalse = (Node*)MakeTerminal(nilNode, TokenPos(&p->lex), 0);
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
    IfNode *node = (IfNode*)MakeNode(ifNode, TokenPos(&p->lex));

    result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->predicate = result.value;

    SkipNewlines(&p->lex);
    if (!MatchToken(TokenArrow, &p->lex)) {
      return ParseFail(ParseError("Expected \"->\"", p), node);
    }
    SkipNewlines(&p->lex);

    result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->ifTrue = result.value;

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!IsOk(result)) return ParseFail(result, node);
    node->ifFalse = result.value;

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
  ListNode *node = (ListNode*)MakeNode(listNode, TokenPos(&p->lex));

  assert(MatchToken(TokenLBracket, &p->lex));
  SkipNewlines(&p->lex);

  if (!MatchToken(TokenRBracket, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!IsOk(result)) return ParseFail(result, node);
      VecPush(node->items, result.value);
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
  ListNode *node = (ListNode*)MakeNode(tupleNode, TokenPos(&p->lex));
  assert(MatchToken(TokenLBrace, &p->lex));

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Result result = ParseExpr(p);
    if (!IsOk(result)) return ParseFail(result, node);
    VecPush(node->items, result.value);

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return Ok(node);
}

static Result ParseVar(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  TerminalNode *node;
  Result result = ParseID(p);
  if (!IsOk(result)) return result;

  node = MakeTerminal(varNode, pos, IDValue(result));
  free(result.value);
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

  if (*token.lexeme == '-') {
    sign = -1;
    token.lexeme++;
    token.length--;
  }

  while (*token.lexeme == '0' || *token.lexeme == '_') {
    token.lexeme++;
    token.length--;
  }

  if (*token.lexeme == 'x') {
    u32 d;
    for (i = 1; i < token.length; i++) {
      if (token.lexeme[i] == '_') continue;
      d = IsDigit(token.lexeme[i])
          ? token.lexeme[i] - '0'
          : 10 + token.lexeme[i] - 'A';
      whole = whole * 16 + d;
    }

    return Ok(MakeTerminal(intNode, pos, sign*whole));
  }

  if (*token.lexeme == '$') {
    u8 byte = token.lexeme[1];
    return Ok(MakeTerminal(intNode, pos, byte));
  }

  for (i = 0; i < token.length; i++) {
    u32 d;
    if (token.lexeme[i] == '_') continue;
    if (token.lexeme[i] == '.') {
      i++;
      break;
    }
    d = token.lexeme[i] - '0';

    if (whole > MaxUInt/10) return ParseError("Number overflows", p);
    whole *= 10;

    if (whole > MaxUInt - d) return ParseError("Number overflows", p);
    whole += d;
  }
  for (; i < token.length; i++) {
    if (token.lexeme[i] == '_') continue;
    frac_size *= 10;
    frac  = frac * 10 + token.lexeme[i] - '0';
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

static Result ParseSymbol(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  TerminalNode *node;
  assert(MatchToken(TokenColon, &p->lex));
  result = ParseID(p);
  if (!IsOk(result)) return result;

  node = MakeTerminal(symbolNode, pos, IDValue(result));
  free(result.value);
  return Ok(node);
}

static Result ParseLiteral(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);

  switch (token.type) {
  case TokenTrue:   return Ok(MakeTerminal(symbolNode, pos, AddSymbol("true")));
  case TokenFalse:  return Ok(MakeTerminal(symbolNode, pos, AddSymbol("false")));
  case TokenNil:    return Ok(MakeTerminal(nilNode, pos, 0));
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
  default:              return nilNode;
  }
}

static NodeType BinaryOpNode(TokenType token_type)
{
  switch (token_type) {
  case TokenBangEq:     return notEqNode;
  case TokenPercent:    return remNode;
  case TokenStar:       return mulNode;
  case TokenPlus:       return addNode;
  case TokenMinus:      return subNode;
  case TokenDot:        return accessNode;
  case TokenSlash:      return divNode;
  case TokenLt:         return ltNode;
  case TokenLtEq:       return ltEqNode;
  case TokenEqEq:       return eqNode;
  case TokenGt:         return gtNode;
  case TokenGtEq:       return gtEqNode;
  case TokenIn:         return inNode;
  case TokenBar:        return pairNode;
  default:              return nilNode;
  }
}
