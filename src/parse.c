#include "parse.h"
#include "mem.h"
#include "reader.h"
#include "printer.h"

typedef enum {
  TOKEN_EOF,
  TOKEN_NEWLINE,
  TOKEN_COMMA,
  TOKEN_COLON,
  TOKEN_DOT,
  TOKEN_PIPE,
  TOKEN_ARROW,
  TOKEN_FAT_ARROW,
  TOKEN_NUMBER,
  TOKEN_SYMBOL,
  TOKEN_BAR,
  TOKEN_PLUS,
  TOKEN_MINUS,
  TOKEN_STAR,
  TOKEN_SLASH,
  TOKEN_EQ,
  TOKEN_NEQ,
  TOKEN_GT,
  TOKEN_GTE,
  TOKEN_LT,
  TOKEN_LTE,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_EXPONENT,
  TOKEN_STRING,
  TOKEN_DO,
  TOKEN_ELSE,
  TOKEN_END,
  TOKEN_COND,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_ERROR,
} TokenType;

typedef struct {
  char *name;
  TokenType type;
} TokenMap;

TokenMap tokens[] = {
  { "and",  TOKEN_AND       },
  { "or",   TOKEN_OR        },
  { "**",   TOKEN_EXPONENT  },
  { "|>",   TOKEN_PIPE      },
  { "->",   TOKEN_ARROW     },
  { "=>",   TOKEN_FAT_ARROW },
  { "!=",   TOKEN_NEQ       },
  { ">=",   TOKEN_GTE       },
  { "<=",   TOKEN_LTE       },
  { "\n",   TOKEN_NEWLINE   },
  { "\r",   TOKEN_NEWLINE   },
  { "|",    TOKEN_BAR       },
  { "+",    TOKEN_PLUS      },
  { "-",    TOKEN_MINUS     },
  { "*",    TOKEN_STAR      },
  { "/",    TOKEN_SLASH     },
  { "=",    TOKEN_EQ        },
  { ">",    TOKEN_GT        },
  { "<",    TOKEN_LT        },
  { "\"",   TOKEN_STRING    },
  { "(",    TOKEN_LPAREN    },
  { ")",    TOKEN_RPAREN    },
  { "[",    TOKEN_LBRACKET  },
  { "]",    TOKEN_RBRACKET  },
  { "{",    TOKEN_LBRACE    },
  { "}",    TOKEN_RBRACE    },
  { ",",    TOKEN_COMMA     },
  { ":",    TOKEN_COLON     },
  { ".",    TOKEN_DOT       },
};

TokenType NextToken(Reader *r);
void DebugParse(Reader *r, char *msg);
void DebugResult(Reader *r, Val result);

Val ParseNumber(Reader *r);
Val ParseIdentifier(Reader *r);
Val ParseSymbol(Reader *r);
Val ParseString(Reader *r);
Val ParseGroup(Reader *r);
Val ParseList(Reader *r);
Val ParseDict(Reader *r);
Val ParseNegative(Reader *r);
Val ParseDo(Reader *r);
Val ParseCond(Reader *r);

Val ParseBlock(Reader *r, Val prefix);
Val ParsePipe(Reader *r, Val prefix);
Val ParseExpr(Reader *r, Val prefix);
Val ParseElse(Reader *r, Val prefix);
Val ParseOperator(Reader *r, Val prefix);
Val ParseLambda(Reader *r, Val prefix);
Val ParseAccess(Reader *r, Val prefix);

typedef enum {
  PREC_NONE,
  PREC_BLOCK,
  PREC_PIPE,
  PREC_EXPR,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_PAIR,
  PREC_TERM,
  PREC_FACTOR,
  PREC_EXPONENT,
  PREC_LAMBDA,
  PREC_NEGATIVE,
  PREC_ACCESS,
  PREC_PRIMARY,
} Precedence;

char *PrecName(Precedence prec)
{
  switch (prec) {
  case PREC_NONE:     return "NONE";
  case PREC_BLOCK:    return "BLOCK";
  case PREC_PIPE:     return "PIPE";
  case PREC_EXPR:     return "EXPR";
  case PREC_LOGIC:    return "LOGIC";
  case PREC_EQUALITY: return "EQUALITY";
  case PREC_COMPARE:  return "COMPARE";
  case PREC_PAIR:     return "PAIR";
  case PREC_TERM:     return "TERM";
  case PREC_FACTOR:   return "FACTOR";
  case PREC_EXPONENT: return "EXPONENT";
  case PREC_LAMBDA:   return "LAMBDA";
  case PREC_NEGATIVE: return "NEGATIVE";
  case PREC_ACCESS:   return "ACCESS";
  case PREC_PRIMARY:  return "PRIMARY";
  }
}

typedef Val (*PrefixFn)(Reader *r);
typedef Val (*InfixFn)(Reader *r, Val prefix);

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Precedence prec;
} ParseRule;

ParseRule rules[] = {
  [TOKEN_EOF] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_NEWLINE] =   { NULL,             ParseBlock,     PREC_BLOCK    },
  [TOKEN_COMMA] =     { NULL,             ParseBlock,     PREC_BLOCK    },
  [TOKEN_COLON] =     { ParseSymbol,      NULL,           PREC_NONE     },
  [TOKEN_DOT] =       { NULL,             ParseAccess,    PREC_ACCESS   },
  [TOKEN_ARROW] =     { NULL,             ParseLambda,    PREC_LAMBDA   },
  [TOKEN_FAT_ARROW] = { NULL,             NULL,           PREC_NONE     },
  [TOKEN_PIPE] =      { ParseIdentifier,  ParsePipe,      PREC_PIPE     },
  [TOKEN_NUMBER] =    { ParseNumber,      ParseExpr,      PREC_EXPR     },
  [TOKEN_SYMBOL] =    { ParseIdentifier,  ParseExpr,      PREC_EXPR     },
  [TOKEN_PLUS] =      { ParseIdentifier,  ParseOperator,  PREC_TERM     },
  [TOKEN_MINUS] =     { ParseNegative,    ParseOperator,  PREC_TERM     },
  [TOKEN_STAR] =      { ParseIdentifier,  ParseOperator,  PREC_FACTOR   },
  [TOKEN_SLASH] =     { ParseIdentifier,  ParseOperator,  PREC_FACTOR   },
  [TOKEN_EXPONENT] =  { ParseIdentifier,  ParseOperator,  PREC_EXPONENT },
  [TOKEN_EQ] =        { ParseIdentifier,  ParseOperator,  PREC_EQUALITY },
  [TOKEN_NEQ] =       { ParseIdentifier,  ParseOperator,  PREC_EQUALITY },
  [TOKEN_GT] =        { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_GTE] =       { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_LT] =        { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_LTE] =       { ParseIdentifier,  ParseOperator,  PREC_COMPARE  },
  [TOKEN_AND] =       { ParseIdentifier,  ParseOperator,  PREC_LOGIC    },
  [TOKEN_OR] =        { ParseIdentifier,  ParseOperator,  PREC_LOGIC    },
  [TOKEN_STRING] =    { ParseString,      ParseExpr,      PREC_EXPR     },
  [TOKEN_DO] =        { ParseDo,          NULL,           PREC_NONE     },
  [TOKEN_ELSE] =      { NULL,             ParseElse,      PREC_EXPR     },
  [TOKEN_END] =       { NULL,             NULL,           PREC_NONE     },
  [TOKEN_COND] =      { ParseCond,        NULL,           PREC_NONE     },
  [TOKEN_BAR] =       { NULL,             ParseOperator,  PREC_PAIR     },
  [TOKEN_LPAREN] =    { ParseGroup,       ParseExpr,      PREC_EXPR     },
  [TOKEN_RPAREN] =    { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACKET] =  { ParseList,        NULL,           PREC_NONE     },
  [TOKEN_RBRACKET] =  { NULL,             NULL,           PREC_NONE     },
  [TOKEN_LBRACE] =    { ParseDict,        ParseExpr,      PREC_EXPR     },
  [TOKEN_RBRACE] =    { NULL,             NULL,           PREC_NONE     },
  [TOKEN_ERROR] =     { NULL,             NULL,           PREC_NONE     },
};

ParseRule *GetRule(TokenType type)
{
  return &rules[type];
}

Val ParseLevel(Reader *r, Precedence level)
{
  SkipSpace(r);
  char *msg = NULL;
  PrintInto(msg, "ParseLevel %s (%d)\n", PrecName(level), level);
  DebugParse(r, msg);
  if (DEBUG_PARSE) PrintSourceContext(r, 0);

  ParseRule *rule = GetRule(NextToken(r));
  if (!rule->prefix) return nil;

  Val exp = rule->prefix(r);
  if (r->status != PARSE_OK) return nil;

  SkipSpace(r);
  rule = GetRule(NextToken(r));
  while (level <= rule->prec) {
    if (DEBUG_PARSE) PrintSourceContext(r, 0);

    exp = rule->infix(r, exp);
    if (r->status != PARSE_OK) return nil;
    SkipSpace(r);
    rule = GetRule(NextToken(r));
  }

  DebugResult(r, exp);
  return exp;
}

Val ParseBlock(Reader *r, Val prefix)
{
  DebugParse(r, "ParseBlock");

  Match(r, ",");
  SkipSpaceAndNewlines(r);

  Val exps = MakePair(prefix, nil);
  while (!IsEnd(Peek(r))) {
    Val exp;

    if (Check(r, "|>")) {
      exp = ParsePipe(r, Head(exps));
      exps = Tail(exps);
    } else {
      exp = ParseLevel(r, PREC_BLOCK + 1);
    }

    if (r->status != PARSE_OK) return nil;
    if (IsNil(exp)) break;

    exps = MakePair(exp, exps);
    SkipSpaceAndNewlines(r);
  }

  exps = Reverse(exps);
  DebugResult(r, exps);
  return exps;
}

Val ParsePipe(Reader *r, Val prefix)
{
  DebugParse(r, "ParsePipe");

  Expect(r, "|>");
  SkipSpace(r);

  Val operand = ParseLevel(r, PREC_PIPE + 1);
  if (r->status != PARSE_OK) return nil;

  Val exp = MakeList(3, MakeSymbol("|>"), prefix, operand);
  DebugResult(r, exp);
  return exp;
}

Val ParseExpr(Reader *r, Val prefix)
{
  DebugParse(r, "ParseExpr");

  Val exps = nil;

  SkipSpace(r);
  while (!IsNewline(Peek(r)) && !Check(r, ",") && !IsEnd(Peek(r))) {
    Val exp = ParseLevel(r, PREC_EXPR + 1);
    if (r->status != PARSE_OK) return nil;
    if (IsNil(exp)) break;

    exps = MakePair(exp, exps);
    SkipSpace(r);
  }

  Val exp = MakePair(prefix, Reverse(exps));
  DebugResult(r, exp);
  return exp;
}

Val ParseElse(Reader *r, Val prefix)
{
  DebugParse(r, "ParseElse");
  Expect(r, "else");

  Val exps = Reverse(prefix);
  Val last_exp = Head(exps);
  if (!Eq(Head(last_exp), SymbolFor("do"))) {
    return ParseError(r, "\"else\" can only follow a \"do\" expression");
  }
  SkipSpaceAndNewlines(r);

  Val else_exp = ParseBlock(r, MakeSymbol("do"));
  if (r->status != PARSE_OK) return nil;

  Expect(r, "end");

  exps = Reverse(MakePair(else_exp, exps));

  DebugResult(r, exps);
  return exps;
}

Val ParseOperator(Reader *r, Val prefix)
{
  DebugParse(r, "ParseOperator");

  ParseRule *rule = GetRule(NextToken(r));

  Val op = ParseIdentifier(r);
  if (r->status != PARSE_OK) return nil;

  Val operand = ParseLevel(r, rule->prec + 1);
  if (r->status != PARSE_OK) return nil;

  Val exp = MakeList(3, op, prefix, operand);
  DebugResult(r, exp);
  return exp;
}

Val ParseLambda(Reader *r, Val prefix)
{
  DebugParse(r, "ParseLambda");
  if (!IsPair(prefix)) {
    prefix = MakePair(prefix, nil);
  }

  Val args = prefix;
  while (!IsNil(args)) {
    if (!IsSym(Head(args))) {
      return ParseError(r, "Only symbols allowed as arguments");
    }
    args = Tail(args);
  }

  Expect(r, "->");
  SkipSpace(r);
  Val body = ParseLevel(r, PREC_EXPR);
  if (r->status != PARSE_OK) return nil;

  Val exp = MakeTagged(3, "->", prefix, body);
  DebugResult(r, exp);
  return exp;
}

Val ParseAccess(Reader *r, Val prefix)
{
  DebugParse(r, "ParseAccess");

  Expect(r, ".");
  Val name = ParseIdentifier(r);
  if (r->status != PARSE_OK) return nil;

  Val key = MakePair(MakeSymbol("quote"), name);
  Val exp = MakeList(2, prefix, key);

  DebugResult(r, exp);
  return exp;
}

Val ParseGroup(Reader *r)
{
  DebugParse(r, "ParseGroup");

  Expect(r, "(");

  Val exp = ParseLevel(r, PREC_EXPR);
  if (r->status != PARSE_OK) return nil;

  Expect(r, ")");
  DebugResult(r, exp);
  return exp;
}

Val ParseList(Reader *r)
{
  DebugParse(r, "ParseList");

  Expect(r, "[");
  SkipSpaceAndNewlines(r);

  Val items = nil;
  while (!Match(r, "]")) {
    Val item = ParseLevel(r, PREC_EXPR);
    if (r->status != PARSE_OK) return nil;

    items = MakePair(item, items);
    Match(r, ",");
    SkipSpaceAndNewlines(r);
  }

  Val exp = MakePair(MakeSymbol("list"), Reverse(items));
  DebugResult(r, exp);
  return exp;
}

Val ParseDict(Reader *r)
{
  DebugParse(r, "ParseDict");

  Expect(r, "{");
  SkipSpaceAndNewlines(r);

  Val keys = nil;
  Val vals = nil;
  while (!Match(r, "}")) {
    if (DEBUG_PARSE) PrintSourceContext(r, 0);

    Val key;
    if (Peek(r) == '"') {
      key = ParseString(r);
      if (r->status != PARSE_OK) return nil;
    } else {
      Val name = ParseIdentifier(r);
      if (r->status != PARSE_OK) return nil;
      key = MakePair(MakeSymbol("quote"), name);
    }
    keys = MakePair(key, keys);

    Expect(r, ":");
    SkipSpaceAndNewlines(r);

    Val val = ParseLevel(r, PREC_EXPR);
    if (r->status != PARSE_OK) return nil;
    vals = MakePair(val, vals);

    SkipSpaceAndNewlines(r);
    Match(r, ",");
    SkipSpaceAndNewlines(r);
  }

  Val exp = MakeTagged(3, "dict", Reverse(keys), Reverse(vals));
  DebugResult(r, exp);
  return exp;
}

Val ParseNegative(Reader *r)
{
  DebugParse(r, "ParseNegative");

  Val exp = ParseLevel(r, PREC_NEGATIVE);
  if (r->status != PARSE_OK) return nil;

  exp = MakeTagged(2, "-", exp);
  DebugResult(r, exp);
  return exp;
}

Val ParseDo(Reader *r)
{
  DebugParse(r, "ParseDo");

  Expect(r, "do");
  SkipSpaceAndNewlines(r);

  Val exp = ParseBlock(r, MakeSymbol("do"));
  if (r->status != PARSE_OK) return nil;

  if (!Check(r, "else")) Expect(r, "end");

  DebugResult(r, exp);
  return exp;
}

Val ParseCond(Reader *r)
{
  DebugParse(r, "ParseCond");

  Expect(r, "cond do");
  SkipSpaceAndNewlines(r);

  Val clauses = nil;
  while (!Check(r, "end") && !Check(r, "else")) {
    Val pred = ParseLevel(r, PREC_EXPR);
    if (r->status != PARSE_OK) return nil;

    SkipSpace(r);
    Expect(r, "=>");
    SkipSpaceAndNewlines(r);

    Val result = ParseLevel(r, PREC_EXPR);
    if (r->status != PARSE_OK) return nil;

    Val clause = MakePair(pred, result);
    clauses = MakePair(clause, clauses);

    SkipSpaceAndNewlines(r);
  }

  if (Match(r, "else")) {
    Val else_exp = ParseBlock(r, MakeSymbol("do"));
    if (r->status != PARSE_OK) return nil;
    Val clause = MakePair(MakeSymbol("true"), else_exp);
    clauses = MakePair(clause, clauses);
  }
  Expect(r, "end");

  Val exp = MakePair(MakeSymbol("cond"), Reverse(clauses));
  DebugResult(r, exp);
  return exp;
}

typedef struct {
  char *name;
  TokenType type;
} Keyword;

Keyword keywords[] = {
  { "do",   TOKEN_DO },
  { "else", TOKEN_ELSE },
  { "end",  TOKEN_END },
  { "cond", TOKEN_COND },
};

TokenType CheckKeywords(Reader *r)
{
  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (CheckToken(r, keywords[i].name)) return keywords[i].type;
  }

  return TOKEN_SYMBOL;
}

TokenType NextToken(Reader *r)
{
  if (IsEnd(Peek(r))) {
    return TOKEN_EOF;
  }

  if (IsDigit(Peek(r))) {
    return TOKEN_NUMBER;
  }

  for (u32 i = 0; i < ArrayCount(tokens); i++) {
    if (Check(r, tokens[i].name)) {
      return tokens[i].type;
    }
  }

  if (IsSymChar(Peek(r))) {
    return CheckKeywords(r);
  }

  ParseError(r, "Expected character");
  return TOKEN_ERROR;
}

Val ParseString(Reader *r)
{
  DebugParse(r, "ParseString");

  Expect(r, "\"");

  u32 start = r->cur;
  while (Peek(r) != '"') {
    if (IsEnd(Peek(r))) return Stop(r);

    if (IsNewline(Peek(r))) {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }

  Val str = MakeBinary(r->src + start, r->cur - start);
  Advance(r);

  DebugResult(r, str);
  return str;
}

Val ParseNumber(Reader *r)
{
  DebugParse(r, "ParseNumber");

  u32 n = Peek(r) - '0';
  Advance(r);
  while (IsDigit(Peek(r))) {
    u32 d = Peek(r) - '0';
    n = n*10 + d;
    Advance(r);
    while (Peek(r) == '_') Advance(r);
  }
  if (Match(r, ".")) {
    if (!IsDigit(Peek(r))) {
      Retreat(r, 1);
      DebugResult(r, IntVal(n));
      return IntVal(n);
    }

    float frac = 0.0;
    float denom = 10.0;
    while (IsDigit(Peek(r))) {
      u32 d = Peek(r) - '0';
      frac += (float)d / denom;
      denom *= 10;
      Advance(r);
      while (Peek(r) == '_') Advance(r);
    }
    DebugResult(r, NumVal(n + frac));
    return NumVal(n + frac);
  }

  DebugResult(r, IntVal(n));
  return IntVal(n);
}

Val ParseIdentifier(Reader *r)
{
  DebugParse(r, "ParseIdentifier");

  u32 start = r->cur;
  while (IsSymChar(Peek(r))) {
    Advance(r);
  }

  if (r->cur == start) {
    ParseError(r, "Expected symbol");
    return nil;
  }

  Val exp = MakeSymbolFromSlice(&r->src[start], r->cur - start);
  DebugResult(r, exp);
  return exp;
}

Val ParseSymbol(Reader *r)
{
  DebugParse(r, "ParseSymbol");
  Expect(r, ":");

  Val name = ParseIdentifier(r);
  if (r->status != PARSE_OK) return nil;

  Val exp = MakePair(MakeSymbol("quote"), name);
  DebugResult(r, exp);
  return exp;
}

Val Parse(Reader *r)
{
  Val exps = nil;
  SkipSpaceAndNewlines(r);
  while (!IsEnd(Peek(r))) {
    Val exp = ParseLevel(r, PREC_NONE + 1);
    if (r->status != PARSE_OK) return nil;

    exps = MakePair(exp, exps);
    SkipSpaceAndNewlines(r);
  }

  return Reverse(exps);
}

void DebugParse(Reader *r, char *msg)
{
  if (DEBUG_PARSE) {
    for (u32 i = 0; i < r->indent; i++) fprintf(stderr, "  ");
    fprintf(stderr, "%s\n", msg);
    r->indent++;
  }
}

void DebugResult(Reader *r, Val result)
{
  if (DEBUG_PARSE) {
    r->indent--;
    for (u32 i = 0; i < r->indent; i++) fprintf(stderr, "  ");
    fprintf(stderr, "=> %s\n", ValStr(result));
  }
}
