#include "parse.h"
#include "project.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"
#include "runtime/primitives.h"

#define ExprNext(lex)       (rules[(lex)->token.type].prefix)
#define PrecNext(lex)       (rules[(lex)->token.type].prec)
#define TokenSym(type)      (rules[type].symbol)
#define ParseOk(val)        OkResult(val)
#define ParseError(msg, p)  ErrorResult(msg, (p)->filename, (p)->lex.token.lexeme - (p)->lex.source)

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

static Result ParseModuleName(Parser *p);
static Result ParseImports(Parser *p);
static Result ParseStmts(Parser *p);
static Result ParseAssign(Parser *p);
static Result ParseDef(Parser *p);
#define ParseExpr(p) ParsePrec(PrecExpr, p)
static Result ParsePrec(Precedence prec, Parser *p);
static Result ParseLeftAssoc(Val prefix, Parser *p);
static Result ParseRightAssoc(Val prefix, Parser *p);
static Result ParseCall(Val prefix, Parser *p);
static Result ParseAccess(Val prefix, Parser *p);
static Result ParseLambda(Parser *p);
static Result ParseUnary(Parser *p);
static Result ParseGroup(Parser *p);
static Result ParseDo(Parser *p);
static Result ParseIf(Parser *p);
static Result ParseCond(Parser *p);
static Result ParseClause(Parser *p);
static Result ParseList(Parser *p);
static Result ParseBraces(Parser *p);
static Result ParseTuple(Parser *p);
static Result ParseMap(Parser *p);
static Result ParseLiteral(Parser *p);
static Result ParseSymbol(Parser *p);
static Result ParseVar(Parser *p);
static Result ParseID(Parser *p);
static Result ParseNum(Parser *p);
static Result ParseString(Parser *p);

typedef Result (*ParseFn)(Parser *p);
typedef Result (*InfixFn)(Val prefix, Parser *p);

typedef struct {
  Val symbol;
  ParseFn prefix;
  InfixFn infix;
  Precedence prec;
} Rule;

static Rule rules[] = {
  [TokenEOF]            = {SymEOF,            0,            0,                  PrecNone},
  [TokenID]             = {SymID,             ParseVar,     0,                  PrecNone},
  [TokenBangEq]         = {SymBangEqual,      0,            ParseLeftAssoc,     PrecEqual},
  [TokenStr]            = {SymString,         ParseString,  0,                  PrecNone},
  [TokenNewline]        = {SymNewline,        0,            0,                  PrecNone},
  [TokenHash]           = {SymHash,           ParseUnary,   0,                  PrecNone},
  [TokenPercent]        = {SymPercent,        0,            ParseLeftAssoc,     PrecProduct},
  [TokenAmpersand]      = {SymAmpersand,      0,            ParseLeftAssoc,     PrecBitwise},
  [TokenLParen]         = {SymLParen,         ParseGroup,   ParseCall,          PrecCall},
  [TokenRParen]         = {SymRParen,         0,            0,                  PrecNone},
  [TokenStar]           = {SymStar,           0,            ParseLeftAssoc,     PrecProduct},
  [TokenPlus]           = {SymPlus,           0,            ParseLeftAssoc,     PrecSum},
  [TokenComma]          = {SymComma,          0,            0,                  PrecNone},
  [TokenMinus]          = {SymMinus,          ParseUnary,   ParseLeftAssoc,     PrecSum},
  [TokenArrow]          = {SymArrow,          0,            0,                  PrecNone},
  [TokenDot]            = {SymDot,            0,            ParseAccess,        PrecAccess},
  [TokenSlash]          = {SymSlash,          0,            ParseLeftAssoc,     PrecProduct},
  [TokenNum]            = {SymNum,            ParseNum,     0,                  PrecNone},
  [TokenColon]          = {SymColon,          ParseSymbol,  0,                  PrecNone},
  [TokenLt]             = {SymLess,           0,            ParseLeftAssoc,     PrecCompare},
  [TokenLtLt]           = {SymLessLess,       0,            ParseLeftAssoc,     PrecShift},
  [TokenLtEq]           = {SymLessEqual,      0,            ParseLeftAssoc,     PrecCompare},
  [TokenLtGt]           = {SymLessGreater,    0,            ParseLeftAssoc,     PrecPair},
  [TokenEq]             = {SymEqual,          0,            0,                  PrecNone},
  [TokenEqEq]           = {SymEqualEqual,     0,            ParseLeftAssoc,     PrecEqual},
  [TokenGt]             = {SymGreater,        0,            ParseLeftAssoc,     PrecCompare},
  [TokenGtEq]           = {SymGreaterEqual,   0,            ParseLeftAssoc,     PrecCompare},
  [TokenGtGt]           = {SymGreaterGreater, 0,            ParseLeftAssoc,     PrecShift},
  [TokenLBracket]       = {SymLBracket,       ParseList,    0,                  PrecNone},
  [TokenBackslash]      = {SymBackslash,      ParseLambda,  0,                  PrecNone},
  [TokenRBracket]       = {SymRBracket,       0,            0,                  PrecNone},
  [TokenCaret]          = {SymCaret,          0,            ParseLeftAssoc,     PrecBitwise},
  [TokenAnd]            = {SymAnd,            0,            ParseLeftAssoc,     PrecLogic},
  [TokenAs]             = {SymAs,             0,            0,                  PrecNone},
  [TokenCond]           = {SymCond,           ParseCond,    0,                  PrecNone},
  [TokenDef]            = {SymDef,            0,            0,                  PrecNone},
  [TokenDo]             = {SymDo,             ParseDo,      0,                  PrecNone},
  [TokenElse]           = {SymElse,           0,            0,                  PrecNone},
  [TokenEnd]            = {SymEnd,            0,            0,                  PrecNone},
  [TokenFalse]          = {SymFalse,          ParseLiteral, 0,                  PrecNone},
  [TokenIf]             = {SymIf,             ParseIf,      0,                  PrecNone},
  [TokenImport]         = {SymImport,         0,            0,                  PrecNone},
  [TokenIn]             = {SymIn,             0,            ParseLeftAssoc,     PrecMember},
  [TokenLet]            = {SymLet,            0,            0,                  PrecNone},
  [TokenModule]         = {SymModule,         0,            0,                  PrecNone},
  [TokenNil]            = {SymNil,            ParseLiteral, 0,                  PrecNone},
  [TokenNot]            = {SymNot,            ParseUnary,   0,                  PrecNone},
  [TokenOr]             = {SymOr,             0,            ParseLeftAssoc,     PrecLogic},
  [TokenTrue]           = {SymTrue,           ParseLiteral, 0,                  PrecNone},
  [TokenLBrace]         = {SymLBrace,         ParseBraces,  0,                  PrecNone},
  [TokenBar]            = {SymBar,            0,            ParseRightAssoc,    PrecPair},
  [TokenRBrace]         = {SymRBrace,         0,            0,                  PrecNone},
  [TokenTilde]          = {SymTilde,          ParseUnary,   0,                  PrecNone}
};

void InitParser(Parser *p, Mem *mem, SymbolTable *symbols)
{
  p->mem = mem;
  p->symbols = symbols;
  p->imports = Nil;
}

Result Parse(char *source, Parser *p)
{
  Result result;
  Val name, body, imports, exports;

  InitLexer(&p->lex, source, 0);
  SkipNewlines(&p->lex);

  result = ParseModuleName(p);
  if (!result.ok) return result;
  name = result.value;

  SkipNewlines(&p->lex);
  result = ParseImports(p);
  if (!result.ok) return result;
  imports = result.value;

  result = ParseStmts(p);
  if (!result.ok) return result;
  if (!MatchToken(TokenEOF, &p->lex)) return ParseError("Unexpected token", p);

  exports = Head(NodeExpr(result.value, p->mem), p->mem);
  body = Tail(NodeExpr(result.value, p->mem), p->mem);

  if (exports == Nil && Tail(body, p->mem) == Nil) {
    body = Head(body, p->mem);
  }

  return ParseOk(MakeModule(name, body, imports, exports, Sym(p->filename, p->symbols), p->mem));
}

static Result ParseModuleName(Parser *p)
{
  if (!MatchToken(TokenModule, &p->lex)) return OkResult(Nil);
  return ParseID(p);
}


static Val DefaultImports(Mem *mem)
{
  PrimitiveModuleDef *primitives = GetPrimitives();
  u32 num_primitives = NumPrimitives();
  u32 i;
  Val imports = Nil;

  for (i = 0; i < num_primitives; i++) {
    Val mod = primitives[i].module;
    Val alias = (i == 0) ? Nil : mod;
    Val import = MakeNode(SymImport, 0, Pair(mod, alias, mem), mem);
    imports = Pair(import, imports, mem);
  }

  return imports;
}

static Result ParseImports(Parser *p)
{
  Val imports = DefaultImports(p->mem);
  while (MatchToken(TokenImport, &p->lex)) {
    Val import, mod, alias;
    u32 pos = TokenPos(&p->lex);
    Result result = ParseID(p);
    if (!result.ok) return result;
    mod = result.value;
    alias = mod;

    if (MatchToken(TokenAs, &p->lex)) {
      if (MatchToken(TokenStar, &p->lex)) {
        alias = Nil;
      } else {
        result = ParseID(p);
        if (!result.ok) return result;
        alias = result.value;
      }
    }

    import = MakeNode(SymImport, pos, Pair(mod, alias, p->mem), p->mem);
    imports = Pair(import, imports, p->mem);
    SkipNewlines(&p->lex);
  }

  return ParseOk(ReverseList(imports, Nil, p->mem));
}

#define EndOfBlock(type)    ((type) == TokenEOF || (type) == TokenEnd || (type) == TokenElse)
static Result ParseStmts(Parser *p)
{
  Val stmts = Nil;
  Val assigns = Nil;
  u32 pos = TokenPos(&p->lex);

  SkipNewlines(&p->lex);
  while (!EndOfBlock(LexPeek(&p->lex))) {
    Result result;
    Val stmt;

    if (MatchToken(TokenLet, &p->lex)) {
      SkipNewlines(&p->lex);
      while (!MatchToken(TokenNewline, &p->lex)) {
        if (MatchToken(TokenEOF, &p->lex)) break;

        result = ParseAssign(p);
        if (!result.ok) return result;

        stmt = MakeNode(SymLet, result.pos, result.value, p->mem);
        stmts = Pair(stmt, stmts, p->mem);
        assigns = Pair(Head(result.value, p->mem), assigns, p->mem);

        if (MatchToken(TokenComma, &p->lex)) SkipNewlines(&p->lex);
      }
    } else if (MatchToken(TokenDef, &p->lex)) {
      result = ParseDef(p);
      if (!result.ok) return result;

      stmt = MakeNode(SymDef, result.pos, result.value, p->mem);
      stmts = Pair(stmt, stmts, p->mem);
      assigns = Pair(Head(result.value, p->mem), assigns, p->mem);
    } else {
      result = ParseExpr(p);
      if (!result.ok) return result;
      stmts = Pair(result.value, stmts, p->mem);
    }

    SkipNewlines(&p->lex);
  }

  if (stmts == Nil) {
    stmts = Pair(MakeNode(SymColon, pos, Nil, p->mem), Nil, p->mem);
  }

  stmts = ReverseList(stmts, Nil, p->mem);
  assigns = ReverseList(assigns, Nil, p->mem);

  return ParseOk(MakeNode(SymDo, pos, Pair(assigns, stmts, p->mem), p->mem));
}

static Result ParseAssign(Parser *p)
{
  Val var;
  Result result;

  result = ParseID(p);
  if (!result.ok) return result;
  var = result.value;

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenEq, &p->lex)) return ParseError("Expected \"=\"", p);
  SkipNewlines(&p->lex);

  result = ParseExpr(p);
  if (!result.ok) return result;

  return ParseOk(Pair(var, result.value, p->mem));
}

static Result ParseDef(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Val var, params = Nil, body, lambda;

  result = ParseID(p);
  if (!result.ok) return result;
  var = result.value;

  if (MatchToken(TokenLParen, &p->lex)) {
    SkipNewlines(&p->lex);
    if (!MatchToken(TokenRParen, &p->lex)) {
      do {
        SkipNewlines(&p->lex);
        result = ParseID(p);
        if (!result.ok) return result;
        params = Pair(result.value, params, p->mem);
      } while (MatchToken(TokenComma, &p->lex));
      SkipNewlines(&p->lex);
      if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \",\" or \")\"", p);
      params = ReverseList(params, Nil, p->mem);
    }
    SkipNewlines(&p->lex);
  }

  result = ParseExpr(p);
  if (!result.ok) return result;
  body = result.value;

  lambda = MakeNode(SymArrow, pos, Pair(params, body, p->mem), p->mem);
  return ParseOk(Pair(var, lambda, p->mem));
}

static Result ParsePrec(Precedence prec, Parser *p)
{
  Result result;

  if (!ExprNext(&p->lex)) return ParseError("Expected expression", p);
  result = ExprNext(&p->lex)(p);

  while (result.ok && PrecNext(&p->lex) >= prec) {
    result = rules[p->lex.token.type].infix(result.value, p);
  }

  return result;
}

static Result ParseLeftAssoc(Val prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Val op = TokenSym(token.type);
  SkipNewlines(&p->lex);
  result = ParsePrec(prec+1, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(prefix, Pair(result.value, Nil, p->mem), p->mem), p->mem));
}

static Result ParseRightAssoc(Val prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Val op = TokenSym(token.type);
  SkipNewlines(&p->lex);
  result = ParsePrec(prec, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(result.value, Pair(prefix, Nil, p->mem), p->mem), p->mem));
}

static Result ParseCall(Val prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Val args = Nil;

  MatchToken(TokenLParen, &p->lex);
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return result;
      args = Pair(result.value, args, p->mem);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \",\" or \")\"", p);
    args = ReverseList(args, Nil, p->mem);
  }

  return ParseOk(MakeNode(SymLParen, pos, Pair(prefix, args, p->mem), p->mem));
}

static Result ParseAccess(Val prefix, Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Precedence prec = rules[token.type].prec;
  Val key;

  if (LexPeek(&p->lex) == TokenID) {
    result = ParseID(p);
    if (!result.ok) return result;
    key = MakeNode(SymColon, pos, result.value, p->mem);
  } else {
    result = ParsePrec(prec+1, p);
    if (!result.ok) return result;
    key = result.value;
  }

  return ParseOk(MakeNode(SymLParen, pos, Pair(prefix, Pair(key, Nil, p->mem), p->mem), p->mem));
}

static Result ParseLambda(Parser *p)
{
  Result result;
  Val params = Nil;
  u32 pos = TokenPos(&p->lex);

  if (!MatchToken(TokenBackslash, &p->lex)) return ParseError("Expected \"\\\"", p);

  SkipNewlines(&p->lex);
  if (!MatchToken(TokenArrow, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseID(p);
      if (!result.ok) return result;
      params = Pair(result.value, params, p->mem);
      SkipNewlines(&p->lex);
    } while (MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenArrow, &p->lex)) return ParseError("Expected \",\" or \"->\"", p);
    params = ReverseList(params, Nil, p->mem);
  }

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(SymArrow, pos, Pair(params, result.value, p->mem), p->mem));
}

static Result ParseUnary(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Val op = TokenSym(token.type);
  result = ParsePrec(PrecUnary, p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(op, pos, Pair(result.value, Nil, p->mem), p->mem));
}

static Result ParseGroup(Parser *p)
{
  Result result;
  Assert(MatchToken(TokenLParen, &p->lex));

  SkipNewlines(&p->lex);
  result = ParseExpr(p);
  if (!result.ok) return result;
  SkipNewlines(&p->lex);
  if (!MatchToken(TokenRParen, &p->lex)) return ParseError("Expected \")\"", p);
  return result;
}

static Result ParseDo(Parser *p)
{
  Result result;
  Val assigns, stmts;
  Assert(MatchToken(TokenDo, &p->lex));

  result = ParseStmts(p);
  if (!result.ok) return result;
  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  assigns = Head(result.value, p->mem);
  stmts = Tail(result.value, p->mem);

  if (assigns == Nil && Tail(stmts, p->mem) == Nil) {
    return OkResult(Head(stmts, p->mem));
  }

  return result;
}

static Result ParseIf(Parser *p)
{
  Result result;
  Val pred, cons = Nil, alt = Nil;
  u32 pos = TokenPos(&p->lex);
  u32 else_pos;

  Assert(MatchToken(TokenIf, &p->lex));

  result = ParseExpr(p);
  if (!result.ok) return result;
  pred = result.value;

  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  result = ParseStmts(p);
  if (!result.ok) return result;
  cons = result.value;

  else_pos = TokenPos(&p->lex);
  if (MatchToken(TokenElse, &p->lex)) {
    result = ParseStmts(p);
    if (!result.ok) return result;
    alt = result.value;
  } else {
    alt = MakeNode(SymNil, else_pos, Nil, p->mem);
  }

  if (!MatchToken(TokenEnd, &p->lex)) return ParseError("Expected \"end\"", p);

  return ParseOk(MakeNode(SymIf, pos,
    Pair(pred,
    Pair(cons,
    Pair(alt, Nil, p->mem), p->mem), p->mem), p->mem));
}

static Result ParseClauses(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);

  if (MatchToken(TokenEnd, &p->lex)) {
    return ParseOk(MakeNode(SymColon, pos, Nil, p->mem));
  } else {
    Val predicate, consequent;

    result = ParseExpr(p);
    if (!result.ok) return result;
    predicate = result.value;

    SkipNewlines(&p->lex);
    if (!MatchToken(TokenArrow, &p->lex)) return ParseError("Expected \"->\"", p);
    SkipNewlines(&p->lex);

    result = ParseExpr(p);
    if (!result.ok) return result;
    consequent = result.value;

    SkipNewlines(&p->lex);
    result = ParseClauses(p);
    if (!result.ok) return result;
    return ParseOk(MakeNode(SymIf, pos,
      Pair(predicate,
      Pair(consequent,
      Pair(result.value, Nil, p->mem), p->mem), p->mem), p->mem));
  }
}

static Result ParseCond(Parser *p)
{
  Assert(MatchToken(TokenCond, &p->lex));
  if (!MatchToken(TokenDo, &p->lex)) return ParseError("Expected \"do\"", p);
  SkipNewlines(&p->lex);
  return ParseClauses(p);
}

static Result ParseList(Parser *p)
{
  Result result;
  Val items = Nil;
  u32 pos = TokenPos(&p->lex);

  Assert(MatchToken(TokenLBracket, &p->lex));
  SkipNewlines(&p->lex);

  if (!MatchToken(TokenRBracket, &p->lex)) {
    do {
      SkipNewlines(&p->lex);
      result = ParseExpr(p);
      if (!result.ok) return result;
      items = Pair(result.value, items, p->mem);
      SkipNewlines(&p->lex);
    } while(MatchToken(TokenComma, &p->lex));
    if (!MatchToken(TokenRBracket, &p->lex)) return ParseError("Expected \")\"", p);
  }

  return ParseOk(MakeNode(SymLBracket, pos, items, p->mem));
}

static Result ParseBraces(Parser *p)
{
  Lexer saved;
  u32 pos = TokenPos(&p->lex);
  Assert(MatchToken(TokenLBrace, &p->lex));
  saved = p->lex;

  if (MatchToken(TokenColon, &p->lex)) {
    SkipNewlines(&p->lex);
    if (MatchToken(TokenRBrace, &p->lex)) {
      return ParseOk(MakeNode(SymRBrace, pos, Nil, p->mem));
    }
  }
  p->lex = saved;

  SkipNewlines(&p->lex);
  if (MatchToken(TokenID, &p->lex)) {
    SkipNewlines(&p->lex);
    if (MatchToken(TokenColon, &p->lex)) {
      p->lex = saved;
      return ParseMap(p);
    }
  }

  p->lex = saved;
  return ParseTuple(p);
}

static Result ParseMap(Parser *p)
{
  Val items = Nil;
  Result result;
  u32 pos = TokenPos(&p->lex);
  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    Val key, value;
    u32 item_pos = TokenPos(&p->lex);

    result = ParseID(p);
    if (!result.ok) return result;
    key = MakeNode(SymColon, item_pos, result.value, p->mem);

    if (!MatchToken(TokenColon, &p->lex)) return ParseError("Expected \":\"", p);

    result = ParseExpr(p);
    if (!result.ok) return result;
    value = result.value;

    value = Pair(key, value, p->mem);
    items = Pair(value, items, p->mem);
    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
  }

  return ParseOk(MakeNode(SymRBrace, pos, items, p->mem));
}

static Result ParseTuple(Parser *p)
{
  Result result;
  Val items = Nil;
  u32 pos = TokenPos(&p->lex);

  SkipNewlines(&p->lex);
  while (!MatchToken(TokenRBrace, &p->lex)) {
    result = ParseExpr(p);
    if (!result.ok) return result;

    MatchToken(TokenComma, &p->lex);
    SkipNewlines(&p->lex);
    items = Pair(result.value, items, p->mem);
  }
  items = ReverseList(items, Nil, p->mem);

  return ParseOk(MakeNode(SymLBrace, pos, items, p->mem));
}

static Result ParseID(Parser *p)
{
  Token token = NextToken(&p->lex);
  if (token.type != TokenID) return ParseError("Expected identifier", p);
  return ParseOk(MakeSymbol(token.lexeme, token.length, p->symbols));
}

static Result ParseVar(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Result result = ParseID(p);
  if (!result.ok) return result;
  return ParseOk(MakeNode(SymID, pos, result.value, p->mem));
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
      d = IsDigit(token.lexeme[i]) ? token.lexeme[i] - '0' : 10 + token.lexeme[i] - 'A';
      whole = whole * 16 + d;
    }
    if (whole > RawInt(MaxIntVal)) return ParseError("Number overflows", p);
    return ParseOk(MakeNode(SymNum, pos, IntVal(sign*whole), p->mem));
  }

  if (*token.lexeme == '$') {
    u8 byte = token.lexeme[1];
    return ParseOk(MakeNode(SymNum, pos, IntVal(byte), p->mem));
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
    return ParseOk(MakeNode(SymNum, pos, FloatVal(sign*num), p->mem));
  } else {
    if (whole > RawInt(MaxIntVal)) return ParseError("Number overflows", p);
    return ParseOk(MakeNode(SymNum, pos, IntVal(sign*whole), p->mem));
  }
}

static Result ParseString(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);
  Val symbol = MakeSymbol(token.lexeme + 1, token.length - 2, p->symbols);
  return ParseOk(MakeNode(SymString, pos, symbol, p->mem));
}

static Result ParseSymbol(Parser *p)
{
  Result result;
  u32 pos = TokenPos(&p->lex);
  Assert(MatchToken(TokenColon, &p->lex));
  result = ParseID(p);
  if (!result.ok) return result;

  return ParseOk(MakeNode(SymColon, pos, result.value, p->mem));
}

static Result ParseLiteral(Parser *p)
{
  u32 pos = TokenPos(&p->lex);
  Token token = NextToken(&p->lex);

  switch (token.type) {
  case TokenTrue:   return ParseOk(MakeNode(SymColon, pos, True, p->mem));
  case TokenFalse:  return ParseOk(MakeNode(SymColon, pos, False, p->mem));
  case TokenNil:    return ParseOk(MakeNode(SymColon, pos, Nil, p->mem));
  default:          return ParseError("Unknown literal", p);
  }
}

Val MakeNode(Val sym, u32 position, Val value, Mem *mem)
{
  Val node;

  if (!CheckMem(mem, 4)) ResizeMem(mem, 2*MemCapacity(mem));

  node = MakeTuple(3, mem);
  TupleSet(node, 0, sym, mem);
  TupleSet(node, 1, IntVal(position), mem);
  TupleSet(node, 2, value, mem);

  return node;
}
