#include "parse.h"

#define ParseOk(val)    ((ParseResult){true, {val}})

/* Function signatures for parsing callbacks */
typedef ParseResult (*PrefixFn)(Lexer *lex, Heap *mem);
typedef ParseResult (*InfixFn)(Val lhs, Lexer *lex, Heap *mem);

/* Operator precedence */
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
  PrecUnary,
  PrecAccess
} Precedence;

/* Rule structure for parsing an expression */
typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence precedence;
} ParseRule;

/* Prefix parsing callback declarations */
static ParseResult ParseID(Lexer *lex, Heap *mem);
static ParseResult ParseString(Lexer *lex, Heap *mem);
static ParseResult ParseUnary(Lexer *lex, Heap *mem);
static ParseResult ParseGroup(Lexer *lex, Heap *mem);
static ParseResult ParseNum(Lexer *lex, Heap *mem);
static ParseResult ParseSymbol(Lexer *lex, Heap *mem);
static ParseResult ParseList(Lexer *lex, Heap *mem);
static ParseResult ParseCond(Lexer *lex, Heap *mem);
static ParseResult ParseDo(Lexer *lex, Heap *mem);
static ParseResult ParseLiteral(Lexer *lex, Heap *mem);
static ParseResult ParseIf(Lexer *lex, Heap *mem);
static ParseResult ParseBraces(Lexer *lex, Heap *mem);

/* Infix parsing callback declarations */
static ParseResult ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem);
static ParseResult ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem);
static ParseResult ParseAccess(Val lhs, Lexer *lex, Heap *mem);

/* Rules for parsing expressions based on the current token type */
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

/* Declarations for statement parsing functions */
static ParseResult ParseStmt(Lexer *lex, Heap *mem);
static ParseResult ParseModuleName(Lexer *lex, Heap *mem);
static ParseResult ParseImport(Lexer *lex, Heap *mem);
static ParseResult ParseLet(Lexer *lex, Heap *mem);
static ParseResult ParseDef(Lexer *lex, Heap *mem);
static ParseResult ParseCall(Lexer *lex, Heap *mem);
static ParseResult ParseExpr(Precedence prec, Lexer *lex, Heap *mem);

/* Helper functions */
static void MakeParseSymbols(Heap *mem);
static ParseResult ParseError(char *message, Lexer *lex);
static void SkipNewlines(Lexer *lex);
static bool MatchToken(TokenType type, Lexer *lex);

/* Parsing entrypoint. This parses a whole script, i.e. a sequence of statements
 * until end-of-file.
 *
 * Parsing is split into statements and expressions. Statements are newline-
 * separated, and are call expressions by default, which will continue to parse
 * call argument expressions until a newline or EOF is reached. Expressions are
 * infix or primary, and are parsed with the precedence parse table above.
 *
 * Each AST node is a pair containing its source position and value, or a list
 * tagged with "error" containing a message and source position.
 */
ParseResult Parse(char *source, Heap *mem)
{
  Lexer lex;
  InitLexer(&lex, source);
  MakeParseSymbols(mem);
  SkipNewlines(&lex);

  /* A script may begin with a module declaration */
  Val mod_name = nil;
  Val mod_pos = TokenPos(&lex);
  if (PeekToken(&lex).type == TokenModule) {
    ParseResult result = ParseModuleName(&lex, mem);
    if (!result.ok) return result;

    mod_name = result.value;
    SkipNewlines(&lex);
  }

  /* Parse each statement */
  Val stmts_pos = TokenPos(&lex);
  Val stmts = nil;
  while (!AtEnd(&lex)) {
    ParseResult stmt = ParseStmt(&lex, mem);
    if (!stmt.ok) return stmt;


    stmts = Pair(stmt.value, stmts, mem);

    /* Statements may be separated by commas */
    MatchToken(TokenComma, &lex);

    SkipNewlines(&lex);
  }

  if (IsNil(mod_name)) {
    Val ast = Pair(stmts_pos, Pair(SymbolFor("do"), ReverseList(stmts, mem), mem), mem);
    return ParseOk(ast);
  } else {
    /* If a module was declared, wrap the script in a module node */
    Val ast =
      Pair(mod_pos,
      Pair(SymbolFor("module"),
      Pair(mod_name,
      Pair(ReverseList(stmts, mem), nil, mem), mem), mem), mem);
    return ParseOk(ast);
  }
}

static ParseResult ParseModuleName(Lexer *lex, Heap *mem)
{
  MatchToken(TokenModule, lex);
  return ParseID(lex, mem);
}

static ParseResult ParseStmt(Lexer *lex, Heap *mem)
{
  ParseResult stmt;

  switch (PeekToken(lex).type) {
  case TokenLet:    stmt = ParseLet(lex, mem); break;
  case TokenDef:    stmt = ParseDef(lex, mem); break;
  case TokenImport: stmt = ParseImport(lex, mem); break;
  default:          stmt = ParseCall(lex, mem); break;
  }

  if (!stmt.ok) return stmt;

  SkipNewlines(lex);

  /* Check for statements we can combine into this one */
  if (IsTagged(Tail(stmt.value, mem), "let", mem)) {
    if (PeekToken(lex).type == TokenLet) {
      ParseResult next_stmt = ParseStmt(lex, mem);
      if (!next_stmt.ok) return next_stmt;
      ListConcat(stmt.value, Tail(Tail(next_stmt.value, mem), mem), mem);
    } else if (PeekToken(lex).type == TokenDef) {
      ParseResult next_stmt = ParseStmt(lex, mem);
      if (!next_stmt.ok) return next_stmt;
      ListConcat(stmt.value, Tail(Tail(next_stmt.value, mem), mem), mem);
    }
  } else if (IsTagged(Tail(stmt.value, mem), "import", mem)) {
    if (PeekToken(lex).type == TokenImport) {
      ParseResult next_stmt = ParseStmt(lex, mem);
      if (!next_stmt.ok) return next_stmt;
      ListConcat(stmt.value, Tail(Tail(next_stmt.value, mem), mem), mem);
    }
  }

  return stmt;
}

static ParseResult ParseImport(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenImport, lex);

  ParseResult name = ParseID(lex, mem);
  if (!name.ok) return name;

  Val alias = name.value;
  if (MatchToken(TokenAs, lex)) {
    ParseResult alias_result = ParseID(lex, mem);
    if (!alias_result.ok) return alias_result;
    alias = alias_result.value;
  }

  return ParseOk(
    Pair(pos,
    Pair(SymbolFor("import"),
    Pair(
      Pair(name.value,
      Pair(alias, nil, mem), mem),
    nil, mem), mem), mem));
}

static ParseResult ParseAssign(Lexer *lex, Heap *mem)
{
  ParseResult id = ParseID(lex, mem);
  if (!id.ok) return id;

  SkipNewlines(lex);

  if (!MatchToken(TokenEqual, lex)) return ParseError("Expected \"=\"", lex);
  SkipNewlines(lex);

  ParseResult value = ParseCall(lex, mem);
  if (!value.ok) return value;
  if (IsNil(value.value)) return ParseError("Expected expression", lex);

  return ParseOk(Pair(id.value, Pair(value.value, nil, mem), mem));
}

static ParseResult ParseLet(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenLet, lex);
  SkipNewlines(lex);

  ParseResult assign = ParseAssign(lex, mem);
  if (!assign.ok) return assign;

  Val assigns = Pair(assign.value, nil, mem);
  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);

    ParseResult assign = ParseAssign(lex, mem);
    if (!assign.ok) return assign;

    assigns = Pair(assign.value, assigns, mem);
  }

  return ParseOk(Pair(pos, Pair(SymbolFor("let"), ReverseList(assigns, mem), mem), mem));
}

static ParseResult ParseDef(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenDef, lex);
  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex);

  ParseResult id = ParseID(lex, mem);
  if (!id.ok) return id;

  Val params_pos = TokenPos(lex);
  Val params = nil;
  while (!MatchToken(TokenRParen, lex)) {
    ParseResult param = ParseID(lex, mem);
    if (!param.ok) return param;

    params = Pair(param.value, params, mem);
  }
  params = Pair(params_pos, ReverseList(params, mem), mem);

  Val body_pos = TokenPos(lex);
  ParseResult body = ParseCall(lex, mem);
  if (!body.ok) return body;

  Val lambda =
    Pair(body_pos,
    Pair(SymbolFor("->"),
    Pair(params,
    Pair(body.value, nil, mem), mem), mem), mem);

  Val assign = Pair(id.value, Pair(lambda, nil, mem), mem);

  return ParseOk(Pair(pos, Pair(SymbolFor("let"), Pair(assign, nil, mem), mem), mem));
}

static ParseResult ParseCall(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  ParseRule rule = GetRule(lex);
  Val args = nil;
  while (rule.prefix != NULL) {
    ParseResult arg = ParseExpr(PrecExpr, lex, mem);
    if (!arg.ok) return arg;

    args = Pair(arg.value, args, mem);
    rule = GetRule(lex);
  }

  if (IsNil(args)) return ParseError("Expected expression", lex);
  if (IsNil(Tail(args, mem))) return ParseOk(Head(args, mem));
  return ParseOk(Pair(pos, ReverseList(args, mem), mem));
}

static ParseResult ParseExpr(Precedence prec, Lexer *lex, Heap *mem)
{
  ParseRule rule = GetRule(lex);
  if (rule.prefix == NULL) return ParseError("Expected expression", lex);

  ParseResult expr = rule.prefix(lex, mem);
  if (!expr.ok) return expr;

  rule = GetRule(lex);
  while (rule.precedence >= prec) {
    expr = rule.infix(expr.value, lex, mem);
    if (!expr.ok) return expr;

    rule = GetRule(lex);
  }

  return expr;
}

static ParseResult ParseID(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  if (lex->token.type != TokenID) return ParseError("Expected identifier", lex);
  Token token = NextToken(lex);
  return ParseOk(Pair(pos, MakeSymbolFrom(token.lexeme, token.length, mem), mem));
}

static ParseResult ParseString(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
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
  return ParseOk(Pair(pos, Pair(SymbolFor("\""), sym, mem), mem));
}

static ParseResult ParseUnary(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  ParseResult rhs = ParseExpr(PrecUnary, lex, mem);
  if (!rhs.ok) return rhs;

  return ParseOk(Pair(pos, Pair(op, Pair(rhs.value, nil, mem), mem), mem));
}

static ParseResult ParseGroup(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex);

  SkipNewlines(lex);
  Val args = nil;
  while (!MatchToken(TokenRParen, lex)) {
    ParseResult arg = ParseExpr(PrecExpr, lex, mem);
    if (!arg.ok) return arg;

    args = Pair(arg.value, args, mem);
    SkipNewlines(lex);
  }

  return ParseOk(Pair(pos, ReverseList(args, mem), mem));
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

static ParseResult ParseNum(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  Token token = NextToken(lex);

  if (token.length > 1 && token.lexeme[1] == 'x') {
    return ParseOk(Pair(pos, IntVal(ParseInt(token.lexeme + 2, token.length - 2, 16)), mem));
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

      return ParseOk(Pair(pos, FloatVal(whole + frac), mem));
    }
  }

  return ParseOk(Pair(pos, IntVal(whole), mem));
}

static ParseResult ParseSymbol(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex);

  ParseResult id = ParseID(lex, mem);
  if (!id.ok) return id;

  return ParseOk(Pair(pos, Pair(SymbolFor(":"), id.value, mem), mem));
}

static ParseResult ParseList(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  if (!MatchToken(TokenLBracket, lex)) return ParseError("Expected \"[\"", lex);
  SkipNewlines(lex);

  if (MatchToken(TokenRBracket, lex)) return ParseOk(Pair(pos, SymbolFor("nil"), mem));

  ParseResult first_item = ParseCall(lex, mem);
  if (!first_item.ok) return first_item;

  Val items = Pair(first_item.value, nil, mem);
  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);
    if (PeekToken(lex).type == TokenRBracket) break;

    ParseResult item = ParseCall(lex, mem);
    if (!item.ok) return item;

    items = Pair(item.value, items, mem);
  }

  if (!MatchToken(TokenRBracket, lex)) return ParseError("Expected \"]\"", lex);

  return ParseOk(Pair(pos, Pair(SymbolFor("["), ReverseList(items, mem), mem), mem));
}

static ParseResult ParseClauses(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  if (MatchToken(TokenEnd, lex)) {
    return ParseOk(Pair(pos, SymbolFor("nil"), mem));
  }

  ParseResult predicate = ParseExpr(PrecLambda+1, lex, mem);
  if (!predicate.ok) return predicate;

  SkipNewlines(lex);
  if (!MatchToken(TokenArrow, lex)) return ParseError("Expected \"->\"", lex);

  SkipNewlines(lex);
  ParseResult consequent = ParseCall(lex, mem);
  if (!consequent.ok) return consequent;

  SkipNewlines(lex);

  ParseResult alternative = ParseClauses(lex, mem);
  if (!alternative.ok) return alternative;

  return ParseOk(
    Pair(pos,
    Pair(SymbolFor("if"),
    Pair(predicate.value,
    Pair(consequent.value,
    Pair(alternative.value, nil, mem), mem), mem), mem), mem));
}

static ParseResult ParseCond(Lexer *lex, Heap *mem)
{
  MatchToken(TokenCond, lex);
  if (!MatchToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex);

  SkipNewlines(lex);

  return ParseClauses(lex, mem);
}

static ParseResult ParseDo(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  MatchToken(TokenDo, lex);
  SkipNewlines(lex);

  Val stmts = nil;
  while (!MatchToken(TokenEnd, lex)) {
    ParseResult stmt = ParseStmt(lex, mem);
    if (!stmt.ok) return stmt;

    stmts = Pair(stmt.value, stmts, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return ParseOk(Pair(pos, Pair(SymbolFor("do"), ReverseList(stmts, mem), mem), mem));
}

static ParseResult ParseLiteral(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  if (MatchToken(TokenTrue, lex)) {
    return ParseOk(Pair(pos, SymbolFor("true"), mem));
  } else if (MatchToken(TokenFalse, lex)) {
    return ParseOk(Pair(pos, SymbolFor("false"), mem));
  } else if (MatchToken(TokenNil, lex)) {
    return ParseOk(Pair(pos, SymbolFor("nil"), mem));
  } else {
    return ParseError("Unknown literal", lex);
  }
}

static ParseResult ParseIf(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenIf, lex);

  ParseResult predicate = ParseExpr(PrecExpr, lex, mem);
  if (!predicate.ok) return predicate;

  Val true_pos = TokenPos(lex);
  if (!MatchToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex);
  SkipNewlines(lex);

  Val true_block = nil;
  while (PeekToken(lex).type != TokenElse && !MatchToken(TokenEnd, lex)) {
    ParseResult stmt = ParseStmt(lex, mem);
    if (!stmt.ok) return stmt;

    true_block = Pair(stmt.value, true_block, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  true_block = Pair(true_pos, Pair(SymbolFor("do"), ReverseList(true_block, mem), mem), mem);

  Val false_block = nil;
  SkipNewlines(lex);

  Val false_pos = TokenPos(lex);
  if (MatchToken(TokenElse, lex)) {
    SkipNewlines(lex);
    while (!MatchToken(TokenEnd, lex)) {
      ParseResult stmt = ParseStmt(lex, mem);
      if (!stmt.ok) return stmt;

      false_block = Pair(stmt.value, false_block, mem);

      MatchToken(TokenComma, lex);
      SkipNewlines(lex);
    }

    false_block = Pair(false_pos, Pair(SymbolFor("do"), ReverseList(false_block, mem), mem), mem);
  } else {
    false_block = Pair(false_pos, SymbolFor("nil"), mem);
  }

  return ParseOk(
    Pair(pos,
    Pair(SymbolFor("if"),
    Pair(predicate.value,
    Pair(true_block,
    Pair(false_block, nil, mem), mem), mem), mem), mem));
}

static ParseResult ParseBraces(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenLBrace, lex);
  SkipNewlines(lex);

  Val items = nil;
  bool is_map = false;

  Lexer saved_lex = *lex;
  if (MatchToken(TokenID, lex) && MatchToken(TokenColon, lex)) {
    is_map = true;
  }
  *lex = saved_lex;

  while (!MatchToken(TokenRBrace, lex)) {
    ParseResult key;
    if (is_map) {
      key = ParseID(lex, mem);
      if (!key.ok) return key;
      if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex);
    }

    ParseResult item = ParseExpr(PrecExpr, lex, mem);
    if (!item.ok) return item;

    if (is_map) {
      items = Pair(Pair(key.value, item.value, mem), items, mem);
    } else {
      items = Pair(item.value, items, mem);
    }
    MatchToken(TokenComma, lex);

    SkipNewlines(lex);
  }

  Val op = (is_map) ? SymbolFor("{:") : SymbolFor("{");

  return ParseOk(Pair(pos, Pair(op, ReverseList(items, mem), mem), mem));
}

static ParseResult ParseAssoc(Val lhs, Precedence prec, Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  ParseResult rhs = ParseExpr(prec, lex, mem);
  if (!rhs.ok) return rhs;

  return ParseOk(Pair(pos, Pair(op, Pair(lhs, Pair(rhs.value, nil, mem), mem), mem), mem));
}

static ParseResult ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  return ParseAssoc(lhs, prec+1, lex, mem);
}

static ParseResult ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  return ParseAssoc(lhs, prec, lex, mem);
}

static ParseResult ParseAccess(Val lhs, Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenDot, lex);
  Val key_pos = TokenPos(lex);
  ParseResult key = ParseID(lex, mem);
  if (!key.ok) return key;

  key.value = Pair(key_pos, Pair(SymbolFor(":"), key.value, mem), mem);
  return ParseOk(Pair(pos, Pair(SymbolFor("."), Pair(lhs, Pair(key.value, nil, mem), mem), mem), mem));
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
  MakeSymbol("{:", mem);
  MakeSymbol(".", mem);
  MakeSymbol("error", mem);
  MakeSymbol("partial", mem);
  MakeSymbol("parse", mem);
  MakeSymbol("module", mem);
}

static ParseResult ParseError(char *message, Lexer *lex)
{
  ParseResult result;
  result.ok = false;
  result.error.message = message;
  result.error.expr = nil;
  result.error.pos = lex->token.lexeme - lex->text;
  return result;
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

typedef struct {
  u32 line;
  u32 col;
} SourceLoc;

static SourceLoc GetLocation(char *source, u32 src_len, u32 pos)
{
  SourceLoc loc = {1, 1};
  for (u32 i = 0; i < pos && i < src_len; i++) {
    if (source[i] == '\n') {
      loc.line++;
      loc.col = 1;
    } else {
      loc.col++;
    }
  }
  return loc;
}

char *ParseErrorMessage(Val error, Heap *mem)
{
  Val pos = ListAt(error, 2, mem);
  Val message = ListAt(error, 3, mem);
  Val source = ListAt(error, 4, mem);
  SourceLoc loc = GetLocation(BinaryData(source, mem), BinaryLength(source, mem), RawInt(pos));

  u32 line_digits = NumDigits(loc.line);
  u32 col_digits = NumDigits(loc.col);
  u32 msg_len = BinaryLength(message, mem);

  char *result = Allocate(1 + line_digits + 1 + col_digits + 2 + msg_len + 1);
  char *cur = result;
  *cur = '[';
  cur++;
  IntToStr(loc.line, cur, line_digits);
  cur += line_digits;
  *cur = ':';
  cur++;
  IntToStr(loc.col, cur, col_digits);
  cur += col_digits;
  *cur = ']';
  cur++;
  *cur = ' ';
  cur++;
  Copy(BinaryData(message, mem), cur, msg_len);
  cur += msg_len;
  *cur = '\0';

  return cur;
}
