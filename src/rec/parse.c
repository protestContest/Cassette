#include "parse.h"
#include "lex.h"

typedef Val (*PrefixFn)(Lexer *lex, Heap *mem);
typedef Val (*InfixFn)(Val lhs, Lexer *lex, Heap *mem);

typedef enum {
  PrecNone,
  PrecExpr,
  PrecLambda,
  PrecOr,
  PrecAnd,
  PrecEqual,
  PrecCompare,
  PrecMember,
  PrecPair,
  PrecSum,
  PrecProduct,
  PrecExponent,
  PrecUnary,
  PrecAccess
} Precedence;

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

static Val ParseID(Lexer *lex, Heap *mem);
static Val ParseString(Lexer *lex, Heap *mem);
static Val ParseUnary(Lexer *lex, Heap *mem);
static Val ParseGroup(Lexer *lex, Heap *mem);
static Val ParseNum(Lexer *lex, Heap *mem);
static Val ParseSymbol(Lexer *lex, Heap *mem);
static Val ParseList(Lexer *lex, Heap *mem);
static Val ParseCond(Lexer *lex, Heap *mem);
static Val ParseDo(Lexer *lex, Heap *mem);
static Val ParseLiteral(Lexer *lex, Heap *mem);
static Val ParseIf(Lexer *lex, Heap *mem);
static Val ParseBraces(Lexer *lex, Heap *mem);

static Val ParseAccess(Val lhs, Lexer *lex, Heap *mem);
static Val ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem);
static Val ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem);

static ParseRule rules[] = {
  [TokenEOF]          = {NULL,          NULL,             PrecNone},
  [TokenID]           = {&ParseID,      NULL,             PrecNone},
  [TokenBangEqual]    = {NULL,          &ParseLeftAssoc,  PrecEqual},
  [TokenString]       = {&ParseString,  NULL,             PrecNone},
  [TokenNewline]      = {NULL,          NULL,             PrecNone},
  [TokenHash]         = {&ParseUnary,   NULL,             PrecNone},
  [TokenPercent]      = {NULL,          &ParseLeftAssoc,  PrecProduct},
  [TokenLParen]       = {&ParseGroup,   NULL,             PrecNone},
  [TokenRParen]       = {NULL,          NULL,             PrecNone},
  [TokenStar]         = {NULL,          &ParseLeftAssoc,  PrecProduct},
  [TokenPlus]         = {NULL,          &ParseLeftAssoc,  PrecSum},
  [TokenComma]        = {NULL,          NULL,             PrecNone},
  [TokenMinus]        = {&ParseUnary,   &ParseLeftAssoc,  PrecSum},
  [TokenArrow]        = {NULL,          &ParseRightAssoc, PrecLambda},
  [TokenDot]          = {NULL,          &ParseAccess,     PrecAccess},
  [TokenSlash]        = {NULL,          &ParseLeftAssoc,  PrecProduct},
  [TokenNum]          = {&ParseNum,     NULL,             PrecNone},
  [TokenColon]        = {&ParseSymbol,  NULL,             PrecNone},
  [TokenLess]         = {NULL,          &ParseLeftAssoc,  PrecCompare},
  [TokenLessEqual]    = {NULL,          &ParseLeftAssoc,  PrecCompare},
  [TokenEqual]        = {NULL,          &ParseLeftAssoc,  PrecEqual},
  [TokenEqualEqual]   = {NULL,          &ParseLeftAssoc,  PrecEqual},
  [TokenGreater]      = {NULL,          &ParseLeftAssoc,  PrecCompare},
  [TokenGreaterEqual] = {NULL,          &ParseLeftAssoc,  PrecCompare},
  [TokenLBracket]     = {&ParseList,    NULL,             PrecNone},
  [TokenRBracket]     = {NULL,          NULL,             PrecNone},
  [TokenAnd]          = {NULL,          &ParseRightAssoc, PrecAnd},
  [TokenAs]           = {NULL,          NULL,             PrecNone},
  [TokenCond]         = {&ParseCond,    NULL,             PrecNone},
  [TokenDef]          = {NULL,          NULL,             PrecNone},
  [TokenDo]           = {&ParseDo,      NULL,             PrecNone},
  [TokenElse]         = {NULL,          NULL,             PrecNone},
  [TokenEnd]          = {NULL,          NULL,             PrecNone},
  [TokenFalse]        = {&ParseLiteral, NULL,             PrecNone},
  [TokenIf]           = {&ParseIf,      NULL,             PrecNone},
  [TokenImport]       = {NULL,          NULL,             PrecNone},
  [TokenIn]           = {NULL,          &ParseLeftAssoc,  PrecMember},
  [TokenLet]          = {NULL,          NULL,             PrecNone},
  [TokenModule]       = {NULL,          NULL,             PrecNone},
  [TokenNil]          = {&ParseLiteral, NULL,             PrecNone},
  [TokenNot]          = {&ParseUnary,   NULL,             PrecNone},
  [TokenOr]           = {NULL,          &ParseRightAssoc, PrecOr},
  [TokenTrue]         = {&ParseLiteral, NULL,             PrecNone},
  [TokenLBrace]       = {&ParseBraces,  NULL,             PrecNone},
  [TokenBar]          = {NULL,          &ParseLeftAssoc,  PrecPair},
  [TokenRBrace]       = {NULL,          NULL,             PrecNone},
};
#define GetRule(lex)    rules[PeekToken(lex).type]

static Val ParseModuleName(Lexer *lex, Heap *mem);
static Val ParseStmt(Lexer *lex, Heap *mem);
static Val ParseImport(Lexer *lex, Heap *mem);
static Val ParseLet(Lexer *lex, Heap *mem);
static Val ParseDef(Lexer *lex, Heap *mem);
static Val ParseCall(Lexer *lex, Heap *mem);
static Val ParseExpr(Precedence prec, Lexer *lex, Heap *mem);

static void MakeParseSymbols(Heap *mem);
static Val ParseError(char *message, Token token, Lexer *lex, Heap *mem);
static Val PartialParse(Heap *mem);
static Val WrapModule(Val stmts, Val name, Heap *mem);
static void SkipNewlines(Lexer *lex);
static bool MatchToken(TokenType type, Lexer *lex);

Val Parse(char *source, Heap *mem)
{
  Lexer lex;
  InitLexer(&lex, source);
  MakeParseSymbols(mem);
  SkipNewlines(&lex);

  Val module = nil;
  if (PeekToken(&lex).type == TokenModule) {
    module = ParseModuleName(&lex, mem);
    if (IsTagged(module, "error", mem)) return module;
  }

  SkipNewlines(&lex);

  Val stmts = nil;
  while (!AtEnd(&lex)) {
    Val stmt = ParseStmt(&lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;
    if (IsNil(stmt)) return ParseError("Expected expression", lex.token, &lex, mem);

    stmts = Pair(stmt, stmts, mem);

    MatchToken(TokenComma, &lex);
    SkipNewlines(&lex);
  }

  if (IsNil(Tail(stmts, mem))) {
    stmts = Head(stmts, mem);
  } else {
    stmts = Pair(SymbolFor("do"), ReverseList(stmts, mem), mem);
  }

  if (IsNil(module)) {
    return stmts;
  } else {
    return WrapModule(stmts, module, mem);
  }

  return stmts;
}

static Val ParseModuleName(Lexer *lex, Heap *mem)
{
  if (!MatchToken(TokenModule, lex)) return ParseError("Expected \"module\"", lex->token, lex, mem);

  return ParseID(lex, mem);
}

static Val ParseStmt(Lexer *lex, Heap *mem)
{
  if (AtEnd(lex)) return PartialParse(mem);

  Val stmt;
  switch (PeekToken(lex).type) {
  case TokenLet:  stmt = ParseLet(lex, mem); break;
  case TokenDef:  stmt = ParseDef(lex, mem); break;
  default:        stmt = ParseCall(lex, mem); break;
  }

  SkipNewlines(lex);
  if (IsTagged(stmt, "let", mem)) {
    if (PeekToken(lex).type == TokenLet) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(next_stmt, mem), mem);
    } else if (PeekToken(lex).type == TokenDef) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(next_stmt, mem), mem);
    }
  } else if (IsTagged(stmt, "import", mem)) {
    if (PeekToken(lex).type == TokenImport) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(next_stmt, mem), mem);
    }
  }

  return stmt;
}

static Val ParseImport(Lexer *lex, Heap *mem)
{
  /*
  import Foo as Bar     [import [Foo Bar]]
  import Foo as *       [import [Foo nil]]
  import Foo            [import [Foo Foo]]
  */

  u32 pos = lex->pos;

  if (!MatchToken(TokenImport, lex)) return ParseError("Expected \"import\"", lex->token, lex, mem);
  if (AtEnd(lex)) return PartialParse(mem);

  Val name = ParseID(lex, mem);
  if (IsTagged(name, "error", mem)) return name;

  Val alias = name;
  if (MatchToken(TokenAs, lex)) {
    if (MatchToken(TokenStar, lex)) {
      alias = nil;
    } else {
      alias = ParseID(lex, mem);
      if (IsTagged(alias, "error", mem)) return alias;
    }
  }

  return
    Pair(SymbolFor("import"),
    Pair(IntVal(pos),
    Pair(name,
    Pair(alias, nil, mem), mem), mem), mem);
}

static Val ParseAssign(Lexer *lex, Heap *mem)
{
  /*
  x = 1     [x 1]
  */

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  if (!MatchToken(TokenEqual, lex)) return ParseError("Expected \"=\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val value = ParseCall(lex, mem);
  if (IsTagged(value, "error", mem)) return value;
  if (IsNil(value)) return ParseError("Expected expression", lex->token, lex, mem);

  return Pair(id, Pair(value, nil, mem), mem);
}

static Val ParseLet(Lexer *lex, Heap *mem)
{
  /*
  let x = 1           [let [x 1]]
  let x = 1, y = 2    [let [x 1] [y 2]]
  */

  // u32 pos = lex->pos;

  if (!MatchToken(TokenLet, lex)) return ParseError("Expected \"let\"", lex->token, lex, mem);
  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  Val assign = ParseAssign(lex, mem);
  if (IsTagged(assign, "error", mem)) return assign;
  Val assigns = Pair(assign, nil, mem);

  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);
    if (AtEnd(lex)) return PartialParse(mem);

    Val assign = ParseAssign(lex, mem);
    if (IsTagged(assign, "error", mem)) return assign;
    assigns = Pair(assign, assigns, mem);
  }

  return
    Pair(SymbolFor("let"), ReverseList(assigns, mem), mem);
}

static Val ParseDef(Lexer *lex, Heap *mem)
{
  /*
  def (foo) 3             [let [foo [-> nil 3]]]
  def (foo x y) bar x y   [let [foo [-> [x y] [bar x y]]]]
  */

  // u32 pos = lex->pos;

  if (!MatchToken(TokenDef, lex)) return ParseError("Expected \"def\"", lex->token, lex, mem);
  if (AtEnd(lex)) return PartialParse(mem);
  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  Val params = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val param = ParseID(lex, mem);
    if (IsTagged(param, "error", mem)) return param;
    params = Pair(param, params, mem);
  }

  Val body = ParseCall(lex, mem);
  if (IsTagged(body, "error", mem)) return body;
  if (IsNil(body)) return ParseError("Expected expression", lex->token, lex, mem);

  Val lambda =
    Pair(SymbolFor("->"),
    Pair(ReverseList(params, mem),
    Pair(body, nil, mem), mem), mem);

  Val assign = Pair(id, Pair(lambda, nil, mem), mem);

  return Pair(SymbolFor("let"), Pair(assign, nil, mem), mem);
}

static Val ParseCall(Lexer *lex, Heap *mem)
{
  /*
  foo x + 1 y   [foo [+ x 1] y]
  foo           foo
  */

  if (AtEnd(lex)) return PartialParse(mem);

  ParseRule rule = GetRule(lex);
  Val args = nil;
  while (rule.prefix != NULL) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    rule = GetRule(lex);
  }

  if (IsNil(Tail(args, mem))) return Head(args, mem);
  return ReverseList(args, mem);
}

static Val ParseExpr(Precedence prec, Lexer *lex, Heap *mem)
{
  if (AtEnd(lex)) return PartialParse(mem);

  ParseRule rule = GetRule(lex);
  if (rule.prefix == NULL) return ParseError("Expected expression", lex->token, lex, mem);

  Val expr = rule.prefix(lex, mem);
  if (IsTagged(expr, "error", mem)) return expr;

  rule = GetRule(lex);
  while (rule.precedence >= prec) {
    expr = rule.infix(expr, lex, mem);
    if (IsTagged(expr, "error", mem)) return expr;
    rule = GetRule(lex);
  }

  return expr;
}

static Val ParseID(Lexer *lex, Heap *mem)
{
  if (AtEnd(lex)) return PartialParse(mem);

  Token token = NextToken(lex);
  if (token.type != TokenID) return ParseError("Expected identifier", token, lex, mem);
  return MakeSymbolFrom(token.lexeme, token.length, mem);
}

static Val ParseString(Lexer *lex, Heap *mem)
{
  /*
  "foo"     [" foo]
  */

  Token token = NextToken(lex);

  char str[token.length+1];
  u32 len = 0;
  for (u32 i = 0, j = 0; i < token.length; i++, j++) {
    if (token.lexeme[i] == '\\') i++;
    str[j] = token.lexeme[i];
    len++;
  }
  str[len] = '\0';

  Val sym = MakeSymbolFrom(str, len, mem);
  return Pair(SymbolFor("\""), sym, mem);
}

static Val ParseUnary(Lexer *lex, Heap *mem)
{
  /*
  #foo      [# foo]
  -foo      [- foo]
  not foo   [not foo]
  */

  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  Val rhs = ParseExpr(PrecUnary, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;
  return Pair(op, Pair(rhs, nil, mem), mem);
}

static Val ParseGroup(Lexer *lex, Heap *mem)
{
  /*
  (foo x + 1 y) [foo [+ x 1] y]
  (foo)         [foo]
  */

  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val args = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    SkipNewlines(lex);
  }

  return ReverseList(args, mem);
}


static u32 ParseDigit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  return 0;
}

static u32 ParseInt(char *lexeme, u32 length, u32 base)
{
  u32 num = 0;
  for (u32 i = 0; i < length; i++) {
    if (lexeme[i] == '_') continue;
    num *= base;
    u32 digit = ParseDigit(lexeme[i]);
    num += digit;
  }

  return num;
}

static Val ParseNum(Lexer *lex, Heap *mem)
{
  /*
  0x1234BEEF
  3.14
  -1.2
  1_000_000
  -4236
  */

  Token token = NextToken(lex);

  if (token.length > 1 && token.lexeme[1] == 'x') {
    return IntVal(ParseInt(token.lexeme + 2, token.length - 2, 16));
  }

  u32 decimal = 0;
  while (decimal < token.length && token.lexeme[decimal] != '.') decimal++;

  u32 whole = ParseInt(token.lexeme, decimal, 10);

  if (decimal < token.length) {
    decimal++;
    for (u32 i = decimal; i < token.length; i++) {
      // float
      float frac = 0;
      float place = 0.1;

      for (u32 j = i; j < token.length; j++) {
        u32 digit = token.lexeme[j] - '0';
        frac += digit * place;
        place /= 10;
      }

      return FloatVal(whole + frac);
    }
  }

  return IntVal(whole);
}

static Val ParseSymbol(Lexer *lex, Heap *mem)
{
  /*
  :foo    [: foo]
  */

  if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);
  if (AtEnd(lex)) return PartialParse(mem);

  Val id = ParseID(lex, mem);
  return Pair(SymbolFor(":"), id, mem);
}

static Val ParseList(Lexer *lex, Heap *mem)
{
  /*
  []        nil
  [1]       [[ 1]
  [1, 2]    [[ 1 2]
  [1, 2,]   [[ 1 2]
  */

  if (!MatchToken(TokenLBracket, lex)) return ParseError("Expected \"[\"", lex->token, lex, mem);
  SkipNewlines(lex);

  if (MatchToken(TokenRBracket, lex)) return SymbolFor("nil");

  Val first_item = ParseCall(lex, mem);
  if (IsTagged(first_item, "error", mem)) return first_item;

  Val items = Pair(first_item, nil, mem);
  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);
    if (PeekToken(lex).type == TokenRBracket) break;

    Val item = ParseCall(lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    items = Pair(item, items, mem);
  }

  if (!MatchToken(TokenRBracket, lex)) return ParseError("Expected \"]\"", lex->token, lex, mem);

  return Pair(SymbolFor("["), ReverseList(items, mem), mem);
}

static Val ParseClauses(Lexer *lex, Heap *mem)
{
  if (MatchToken(TokenElse, lex)) {
    Val alternative = ParseCall(lex, mem);
    if (IsTagged(alternative, "error", mem)) return alternative;
    SkipNewlines(lex);
    if (!MatchToken(TokenEnd, lex)) return ParseError("Expected \"end\"", lex->token, lex, mem);
    return alternative;
  }

  Val predicate = ParseExpr(PrecLambda+1, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  SkipNewlines(lex);
  if (!MatchToken(TokenArrow, lex)) return ParseError("Expected \"->\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val consequent = ParseCall(lex, mem);
  if (IsTagged(consequent, "error", mem)) return consequent;
  if (IsNil(consequent)) return ParseError("Expected expression", lex->token, lex, mem);

  SkipNewlines(lex);

  Val alternative = ParseClauses(lex, mem);

  return
    Pair(SymbolFor("if"),
    Pair(predicate,
    Pair(consequent,
    Pair(alternative, nil, mem), mem), mem), mem);
}

static Val ParseCond(Lexer *lex, Heap *mem)
{
  /*
  cond
    x == 3   -> :three
    x == 2   -> :two
  else
    :err
  end

  [if [== x 3] :three [if [== x 2] :two :err]]
  */

  if (!MatchToken(TokenCond, lex)) return ParseError("Expected \"cond\"", lex->token, lex, mem);

  SkipNewlines(lex);
  if (AtEnd(lex)) return PartialParse(mem);

  return ParseClauses(lex, mem);
}

static Val ParseDo(Lexer *lex, Heap *mem)
{
  /*
  do
    foo x y
    bar z
    baz
  end
  ; => [do [foo x y] [bar z] baz]

  do
    foo x
  end
  ; => [foo x]
  */

  if (!MatchToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val stmts = nil;
  while (!MatchToken(TokenEnd, lex)) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;
    if (IsNil(stmt)) return ParseError("Expected expression", lex->token, lex, mem);

    stmts = Pair(stmt, stmts, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  if (ListLength(stmts, mem) == 1) return Head(stmts, mem);

  return Pair(SymbolFor("do"), ReverseList(stmts, mem), mem);
}

static Val ParseLiteral(Lexer *lex, Heap *mem)
{
  Token token = NextToken(lex);
  switch (token.type) {
  case TokenTrue:   return SymbolFor("true");
  case TokenFalse:  return SymbolFor("false");
  case TokenNil:    return SymbolFor("nil");
  default:          return ParseError("Unknown literal", token, lex, mem);
  }
}

static Val ParseIf(Lexer *lex, Heap *mem)
{
  /*
  if x % 2 == 0 do :ok else :err end
  ; => [if [== [% x 2] 0] :ok :err]
  */

  if (!MatchToken(TokenIf, lex)) return ParseError("Expected \"if\"", lex->token, lex, mem);

  Val predicate = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  if (!MatchToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val true_block = nil;
  while (!MatchToken(TokenElse, lex)) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;
    if (IsNil(stmt)) return ParseError("Expected expression", lex->token, lex, mem);

    true_block = Pair(stmt, true_block, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  if (ListLength(true_block, mem) == 1) {
    true_block = Head(true_block, mem);
  } else {
    true_block = ReverseList(true_block, mem);
  }

  Val false_block = nil;
  SkipNewlines(lex);

  while (!MatchToken(TokenEnd, lex)) {
    if (AtEnd(lex)) return PartialParse(mem);

    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;
    if (IsNil(stmt)) return ParseError("Expected expression", lex->token, lex, mem);

    false_block = Pair(stmt, false_block, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  if (ListLength(false_block, mem) == 1) {
    false_block = Head(false_block, mem);
  } else {
    false_block = ReverseList(false_block, mem);
  }

  return
    Pair(SymbolFor("if"),
    Pair(predicate,
    Pair(true_block,
    Pair(false_block, nil, mem), mem), mem), mem);
}

static Val ParseMap(Val items, Lexer *lex, Heap *mem)
{
  while (!MatchToken(TokenRBrace, lex)) {
    Val key = ParseID(lex, mem);
    if (IsTagged(key, "error", mem)) return key;

    if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);

    SkipNewlines(lex);
    Val val = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(val, "error", mem)) return val;

    Val item = Pair(key, val, mem);
    items = Pair(item, items, mem);
    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return Pair(MakeSymbol("{:", mem), ReverseList(items, mem), mem);
}

static Val ParseBraces(Lexer *lex, Heap *mem)
{
  if (!MatchToken(TokenLBrace, lex)) return ParseError("Expected \"{\"", lex->token, lex, mem);

  SkipNewlines(lex);

  if (MatchToken(TokenRBrace, lex)) {
    // empty tuple
    return Pair(SymbolFor("{"), nil, mem);
  }

  Lexer saved_lex = *lex;
  Token first_token = lex->token;
  if (MatchToken(TokenID, lex) && MatchToken(TokenColon, lex)) {
    // parse as a map
    Val key = MakeSymbolFrom(first_token.lexeme, first_token.length, mem);

    Val value = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(value, "error", mem)) return value;

    Val items = Pair(Pair(key, value, mem), nil, mem);
    if (!MatchToken(TokenComma, lex)) return ParseError("Expected \",\"", lex->token, lex, mem);

    SkipNewlines(lex);
    return ParseMap(items, lex, mem);
  }

  // not a map, normal tuple
  // backtrack
  *lex = saved_lex;
  Val items = nil;
  while (!MatchToken(TokenRBrace, lex)) {
    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    items = Pair(item, items, mem);
    MatchToken(TokenComma, lex);
    if (!MatchToken(TokenComma, lex)) return ParseError("Expected \",\"", lex->token, lex, mem);

    SkipNewlines(lex);
  }

  return Pair(SymbolFor("{"), ReverseList(items, mem), mem);
}

static Val ParseAccess(Val lhs, Lexer *lex, Heap *mem)
{
  if (!MatchToken(TokenDot, lex)) return ParseError("Expected \".\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  Val key = Pair(SymbolFor(":"), id, mem);
  return
    Pair(SymbolFor("."),
    Pair(lhs,
    Pair(key, nil, mem), mem), mem);
}

static Val ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  Val rhs = ParseExpr(prec + 1, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

static Val ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  Val rhs = ParseExpr(prec, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem);
}

static void MakeParseSymbols(Heap *mem)
{
  MakeSymbol("do", mem);
  MakeSymbol("import", mem);
  MakeSymbol("let", mem);
  MakeSymbol("->", mem);
  MakeSymbol("\"", mem);
  MakeSymbol(":", mem);
  MakeSymbol("[", mem);
  MakeSymbol("if", mem);
  MakeSymbol("true", mem);
  MakeSymbol("false", mem);
  MakeSymbol("nil", mem);
  MakeSymbol("{", mem);
  MakeSymbol(".", mem);
  MakeSymbol("error", mem);
  MakeSymbol("partial", mem);
  MakeSymbol("parse", mem);
  MakeSymbol("module", mem);
}

static Val ParseError(char *message, Token token, Lexer *lex, Heap *mem)
{
  return
    Pair(SymbolFor("error"),
    Pair(SymbolFor("parse"),
    Pair(BinaryFrom(message, StrLen(message), mem),
    Pair(BinaryFrom(lex->text, StrLen(lex->text), mem), nil, mem), mem), mem), mem);
}

static Val PartialParse(Heap *mem)
{
  return Pair(SymbolFor("error"), SymbolFor("partial"), mem);
}

static Val WrapModule(Val stmts, Val name, Heap *mem)
{
  return
    Pair(SymbolFor("module"),
    Pair(name,
    Pair(stmts, nil, mem), mem), mem);
}

static void SkipNewlines(Lexer *lex)
{
  while (MatchToken(TokenNewline, lex)) ;
}

static bool MatchToken(TokenType type, Lexer *lex)
{
  if (PeekToken(lex).type == type) {
    NextToken(lex);
    return true;
  }
  return false;
}
