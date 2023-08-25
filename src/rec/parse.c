#include "parse.h"

/* Function signatures for parsing callbacks */
typedef Val (*PrefixFn)(Lexer *lex, Heap *mem);
typedef Val (*InfixFn)(Val lhs, Lexer *lex, Heap *mem);

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

/* Infix parsing callback declarations */
static Val ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem);
static Val ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem);
static Val ParseAccess(Val lhs, Lexer *lex, Heap *mem);

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
static Val ParseModuleName(Lexer *lex, Heap *mem);
static Val ParseImport(Lexer *lex, Heap *mem);
static Val ParseLet(Lexer *lex, Heap *mem);
static Val ParseDef(Lexer *lex, Heap *mem);
static Val ParseCall(Lexer *lex, Heap *mem);
static Val ParseExpr(Precedence prec, Lexer *lex, Heap *mem);

/* Helper functions */
static void MakeParseSymbols(Heap *mem);
static Val ParseError(char *message, Token token, Lexer *lex, Heap *mem);
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
Val Parse(char *source, Heap *mem)
{
  Lexer lex;
  InitLexer(&lex, source);
  MakeParseSymbols(mem);
  SkipNewlines(&lex);

  /* A script may begin with a module declaration */
  Val module = nil;
  Val mod_pos = TokenPos(&lex);
  if (PeekToken(&lex).type == TokenModule) {
    module = ParseModuleName(&lex, mem);
    if (IsTagged(module, "error", mem)) return module;
    SkipNewlines(&lex);
  }

  /* Parse each statement */
  Val stmts_pos = TokenPos(&lex);
  Val stmts = nil;
  while (!AtEnd(&lex)) {
    Val stmt = ParseStmt(&lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    stmts = Pair(stmt, stmts, mem);

    /* Statements may be separated by commas */
    MatchToken(TokenComma, &lex);

    SkipNewlines(&lex);
  }


  if (IsNil(module)) {
    return Pair(stmts_pos, Pair(SymbolFor("do"), ReverseList(stmts, mem), mem), mem);
  } else {
    /* If a module was declared, wrap the script in a module node */
    return
      Pair(mod_pos,
      Pair(SymbolFor("module"),
      Pair(module,
      Pair(stmts, nil, mem), mem), mem), mem);
  }
}

/* Syntax: "module" ID
 * AST: [module ID node]
 */
static Val ParseModuleName(Lexer *lex, Heap *mem)
{
  MatchToken(TokenModule, lex);

  return ParseID(lex, mem);
}

/* Parses a statement: "let", "def", "import", or a call expression. Consecutive
 * "let"s and "def"s are combined into a single "let" node.
 */
Val ParseStmt(Lexer *lex, Heap *mem)
{
  Val stmt;

  switch (PeekToken(lex).type) {
  case TokenLet:    stmt = ParseLet(lex, mem); break;
  case TokenDef:    stmt = ParseDef(lex, mem); break;
  case TokenImport: stmt = ParseImport(lex, mem); break;
  default:          stmt = ParseCall(lex, mem); break;
  }

  SkipNewlines(lex);

  /* Check for statements we can combine into this one */
  if (IsTagged(Tail(stmt, mem), "let", mem)) {
    if (PeekToken(lex).type == TokenLet) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(Tail(next_stmt, mem), mem), mem);
    } else if (PeekToken(lex).type == TokenDef) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(Tail(next_stmt, mem), mem), mem);
    }
  } else if (IsTagged(Tail(stmt, mem), "import", mem)) {
    if (PeekToken(lex).type == TokenImport) {
      Val next_stmt = ParseStmt(lex, mem);
      ListConcat(stmt, Tail(Tail(next_stmt, mem), mem), mem);
    }
  }

  return stmt;
}

/* Syntax:
 *    "import" ID "as" ID
 *    "import" ID "as" "*"
 *    "import" ID
 * AST:
 *    [import ID ID]
 *    [import ID nil]
 */
static Val ParseImport(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenImport, lex);

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

  return Pair(pos,
    Pair(SymbolFor("import"),
    Pair(
      Pair(name,
      Pair(alias, nil, mem), mem),
    nil, mem), mem), mem);
}

/* Syntax: ID "=" call
 * AST: [ID call]
 */
static Val ParseAssign(Lexer *lex, Heap *mem)
{
  /*
  x = 1     [x 1]
  */

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  SkipNewlines(lex);

  if (!MatchToken(TokenEqual, lex)) return ParseError("Expected \"=\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val value = ParseCall(lex, mem);
  if (IsTagged(value, "error", mem)) return value;
  if (IsNil(value)) return ParseError("Expected expression", lex->token, lex, mem);

  return Pair(id, Pair(value, nil, mem), mem);
}

/* Syntax: "let" assign ("," assign)*
 * AST: [let [assigns]]
 */
static Val ParseLet(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenLet, lex);
  SkipNewlines(lex);

  Val assign = ParseAssign(lex, mem);
  if (IsTagged(assign, "error", mem)) return assign;
  Val assigns = Pair(assign, nil, mem);

  while (MatchToken(TokenComma, lex)) {
    SkipNewlines(lex);

    Val assign = ParseAssign(lex, mem);
    if (IsTagged(assign, "error", mem)) return assign;
    assigns = Pair(assign, assigns, mem);
  }

  return Pair(pos, Pair(SymbolFor("let"), ReverseList(assigns, mem), mem), mem);
}

/* Syntax: "def" "(" ID (ID)* ")" expr
 * AST: [let [ID [-> [params] expr]]]
 */
static Val ParseDef(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenDef, lex);
  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  if (IsTagged(id, "error", mem)) return id;

  Val params_pos = TokenPos(lex);
  Val params = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val param = ParseID(lex, mem);
    if (IsTagged(param, "error", mem)) return param;
    params = Pair(param, params, mem);
  }
  params = Pair(params_pos, ReverseList(params, mem), mem);

  Val body_pos = TokenPos(lex);
  Val body = ParseCall(lex, mem);
  if (IsTagged(body, "error", mem)) return body;

  Val lambda =
    Pair(body_pos,
    Pair(SymbolFor("->"),
    Pair(params,
    Pair(body, nil, mem), mem), mem), mem);

  Val assign = Pair(id, Pair(lambda, nil, mem), mem);

  return Pair(pos, Pair(SymbolFor("let"), Pair(assign, nil, mem), mem), mem);
}

static Val ParseCall(Lexer *lex, Heap *mem)
{
  /*
  foo x + 1 y   [foo [+ x 1] y]
  foo           foo
  */

  Val pos = TokenPos(lex);
  ParseRule rule = GetRule(lex);
  Val args = nil;
  while (rule.prefix != NULL) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    rule = GetRule(lex);
  }

  if (IsNil(args)) return ParseError("Expected expression", lex->token, lex, mem);
  if (IsNil(Tail(args, mem))) return Head(args, mem);
  return Pair(pos, ReverseList(args, mem), mem);
}

static Val ParseExpr(Precedence prec, Lexer *lex, Heap *mem)
{
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
  Val pos = TokenPos(lex);
  Token token = NextToken(lex);
  if (token.type != TokenID) return ParseError("Expected identifier", token, lex, mem);
  return Pair(pos, MakeSymbolFrom(token.lexeme, token.length, mem), mem);
}

static Val ParseString(Lexer *lex, Heap *mem)
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
  return Pair(pos, Pair(SymbolFor("\""), sym, mem), mem);
}

static Val ParseUnary(Lexer *lex, Heap *mem)
{
  /*
  #foo      [# foo]
  -foo      [- foo]
  not foo   [not foo]
  */

  Val pos = TokenPos(lex);
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  Val rhs = ParseExpr(PrecUnary, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;
  return Pair(pos, Pair(op, Pair(rhs, nil, mem), mem), mem);
}

static Val ParseGroup(Lexer *lex, Heap *mem)
{
  /*
  (foo x + 1 y) [foo [+ x 1] y]
  (foo)         [foo]
  */

  Val pos = TokenPos(lex);
  if (!MatchToken(TokenLParen, lex)) return ParseError("Expected \"(\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val args = nil;
  while (!MatchToken(TokenRParen, lex)) {
    Val arg = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(arg, "error", mem)) return arg;

    args = Pair(arg, args, mem);
    SkipNewlines(lex);
  }

  return Pair(pos, ReverseList(args, mem), mem);
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
  1_000_000
  */

  Val pos = TokenPos(lex);
  Token token = NextToken(lex);

  if (token.length > 1 && token.lexeme[1] == 'x') {
    return Pair(pos, IntVal(ParseInt(token.lexeme + 2, token.length - 2, 16)), mem);
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

      return Pair(pos, FloatVal(whole + frac), mem);
    }
  }

  return Pair(pos, IntVal(whole), mem);
}

static Val ParseSymbol(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);

  Val id = ParseID(lex, mem);
  return Pair(pos, Pair(SymbolFor(":"), id, mem), mem);
}

static Val ParseList(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  if (!MatchToken(TokenLBracket, lex)) return ParseError("Expected \"[\"", lex->token, lex, mem);
  SkipNewlines(lex);

  if (MatchToken(TokenRBracket, lex)) return Pair(pos, SymbolFor("nil"), mem);

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

  return Pair(pos, Pair(SymbolFor("["), ReverseList(items, mem), mem), mem);
}

static Val ParseClauses(Lexer *lex, Heap *mem)
{
  if (MatchToken(TokenEnd, lex)) {
    return nil;
  }
  // if (MatchToken(TokenElse, lex)) {
  //   Val alternative = ParseCall(lex, mem);
  //   if (IsTagged(alternative, "error", mem)) return alternative;
  //   SkipNewlines(lex);
  //   if (!MatchToken(TokenEnd, lex)) return ParseError("Expected \"end\"", lex->token, lex, mem);
  //   return alternative;
  // }

  Val predicate = ParseExpr(PrecLambda+1, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  SkipNewlines(lex);
  if (!MatchToken(TokenArrow, lex)) return ParseError("Expected \"->\"", lex->token, lex, mem);

  SkipNewlines(lex);
  Val consequent = ParseCall(lex, mem);
  if (IsTagged(consequent, "error", mem)) return consequent;

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

  Val pos = TokenPos(lex);
  MatchToken(TokenCond, lex);

  SkipNewlines(lex);

  return Pair(pos, ParseClauses(lex, mem), mem);
}

static Val ParseDo(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  MatchToken(TokenDo, lex);
  SkipNewlines(lex);

  Val stmts = nil;
  while (!MatchToken(TokenEnd, lex)) {
    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    stmts = Pair(stmt, stmts, mem);

    MatchToken(TokenComma, lex);
    SkipNewlines(lex);
  }

  return Pair(pos, Pair(SymbolFor("do"), ReverseList(stmts, mem), mem), mem);
}

static Val ParseLiteral(Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);

  Token token = NextToken(lex);
  switch (token.type) {
  case TokenTrue:   return Pair(pos, SymbolFor("true"), mem);
  case TokenFalse:  return Pair(pos, SymbolFor("false"), mem);
  case TokenNil:    return Pair(pos, SymbolFor("nil"), mem);
  default:          return ParseError("Unknown literal", token, lex, mem);
  }
}

static Val ParseIf(Lexer *lex, Heap *mem)
{
  /*
  if x % 2 == 0 do :ok else :err end
  ; => [if [== [% x 2] 0] :ok :err]
  */

  Val pos = TokenPos(lex);
  MatchToken(TokenIf, lex);

  Val predicate = ParseExpr(PrecExpr, lex, mem);
  if (IsTagged(predicate, "error", mem)) return predicate;

  Val true_pos = TokenPos(lex);
  if (!MatchToken(TokenDo, lex)) return ParseError("Expected \"do\"", lex->token, lex, mem);
  SkipNewlines(lex);

  Val true_block = nil;
  while (PeekToken(lex).type != TokenElse && !MatchToken(TokenEnd, lex)) {
    Val stmt = ParseStmt(lex, mem);
    if (IsTagged(stmt, "error", mem)) return stmt;

    true_block = Pair(stmt, true_block, mem);

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
      Val stmt = ParseStmt(lex, mem);
      if (IsTagged(stmt, "error", mem)) return stmt;

      false_block = Pair(stmt, false_block, mem);

      MatchToken(TokenComma, lex);
      SkipNewlines(lex);
    }

    false_block = Pair(false_pos, Pair(SymbolFor("do"), ReverseList(false_block, mem), mem), mem);
  }

  return Pair(pos,
    Pair(SymbolFor("if"),
    Pair(predicate,
    Pair(true_block,
    Pair(false_block, nil, mem), mem), mem), mem), mem);
}

static Val ParseBraces(Lexer *lex, Heap *mem)
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
    Val key;
    if (is_map) {
      key = ParseID(lex, mem);
      if (IsTagged(key, "error", mem)) return key;
      if (!MatchToken(TokenColon, lex)) return ParseError("Expected \":\"", lex->token, lex, mem);
    }

    Val item = ParseExpr(PrecExpr, lex, mem);
    if (IsTagged(item, "error", mem)) return item;

    if (is_map) {
      items = Pair(Pair(key, item, mem), items, mem);
    } else {
      items = Pair(item, items, mem);
    }
    MatchToken(TokenComma, lex);

    SkipNewlines(lex);
  }

  Val op = (is_map) ? SymbolFor("{:") : SymbolFor("{");

  return Pair(pos, Pair(op, ReverseList(items, mem), mem), mem);
}

static Val ParseAssoc(Val lhs, Precedence prec, Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  Token token = NextToken(lex);
  Val op = MakeSymbolFrom(token.lexeme, token.length, mem);
  SkipNewlines(lex);
  Val rhs = ParseExpr(prec, lex, mem);
  if (IsTagged(rhs, "error", mem)) return rhs;

  return Pair(pos, Pair(op, Pair(lhs, Pair(rhs, nil, mem), mem), mem), mem);
}

static Val ParseLeftAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  return ParseAssoc(lhs, prec+1, lex, mem);
}

static Val ParseRightAssoc(Val lhs, Lexer *lex, Heap *mem)
{
  Precedence prec = GetRule(lex).precedence;
  return ParseAssoc(lhs, prec, lex, mem);
}

static Val ParseAccess(Val lhs, Lexer *lex, Heap *mem)
{
  Val pos = TokenPos(lex);
  MatchToken(TokenDot, lex);
  Val key_pos = TokenPos(lex);
  Val key = ParseID(lex, mem);
  if (IsTagged(key, "error", mem)) return key;

  key = Pair(key_pos, Pair(SymbolFor(":"), key, mem), mem);
  return Pair(pos, Pair(SymbolFor("."), Pair(lhs, Pair(key, nil, mem), mem), mem), mem);
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

static Val ParseError(char *message, Token token, Lexer *lex, Heap *mem)
{
  Val pos = IntVal(token.lexeme - lex->text);

  return
    Pair(SymbolFor("error"),
    Pair(SymbolFor("parse"),
    Pair(pos,
    Pair(BinaryFrom(message, StrLen(message), mem),
    Pair(BinaryFrom(lex->text, StrLen(lex->text), mem), nil, mem), mem), mem), mem), mem);
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
