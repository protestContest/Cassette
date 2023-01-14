#include "parse.h"
#include "mem.h"
#include "reader.h"
#include "printer.h"

#define DEBUG_PARSE 1

typedef enum {
  PREC_EXPR,
  PREC_LAMBDA,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_RANGE,
  PREC_TERM,
  PREC_FACTOR,
  PREC_EXPONENT,
  PREC_CONS,
  PREC_NEGATIVE,
  PREC_LOOKUP,
  PREC_PRIMARY,
} Precedence;

typedef Val (*PrefixFn)(Reader *r);
typedef Val (*InfixFn)(Reader *r, Val prefix);

typedef struct {
  char *intro;
  PrefixFn parse;
} PrefixRule;

typedef struct {
  char *name;
  Precedence prec;
  InfixFn parse;
} Operator;

Val ParseBlock(Reader *r);
Val ParsePipe(Reader *r);
Val ParseExpr(Reader *r);
Val ParseDoBlock(Reader *r);
Val ParseElseBlock(Reader *r);
Val ParseCond(Reader *r);

Val ParseLookup(Reader *r, Val prefix);
Val ParseLambda(Reader *r, Val prefix);
Val ParseNegative(Reader *r);

Val ParseGroup(Reader *r);
Val ParseList(Reader *r);
Val ParseTuple(Reader *r);
Val ParseDict(Reader *r);
Val ParseNumber(Reader *r);
Val ParseString(Reader *r);
Val ParseIdentifier(Reader *r);
Val ParseSymbol(Reader *r);

static PrefixRule prefix_rules[] = {
  { "cond", &ParseCond      },
  { "do",   &ParseDoBlock   },
  { "#[",   &ParseTuple     },
  { "[",    &ParseList      },
  { "{",    &ParseDict      },
  { "(",    &ParseGroup     },
  { "-",    &ParseNegative  },
  { "\"",   &ParseString    },
  { ":",    &ParseSymbol    },
};

static Operator ops[] = {
  { "and",  PREC_LOGIC,     NULL },
  { "or",   PREC_LOGIC,     NULL },
  { "->",   PREC_LAMBDA,    ParseLambda },
  { "..",   PREC_RANGE,     NULL },
  { "**",   PREC_EXPONENT,  NULL },
  { ">=",   PREC_COMPARE,   NULL },
  { "<=",   PREC_COMPARE,   NULL },
  { "!=",   PREC_EQUALITY,  NULL },
  { ".",    PREC_LOOKUP,    ParseLookup },
  { "*",    PREC_FACTOR,    NULL },
  { "/",    PREC_FACTOR,    NULL },
  { "+",    PREC_TERM,      NULL },
  { "-",    PREC_TERM,      NULL },
  { ">",    PREC_COMPARE,   NULL },
  { "<",    PREC_COMPARE,   NULL },
  { "=",    PREC_EQUALITY,  NULL },
  { "|",    PREC_CONS,      NULL },
};

Val ParseInfix(Reader *r, Precedence prec);
PrefixRule *MatchPrefix(Reader *r);
Operator *MatchOperator(Reader *r, Precedence prec);

bool IsSymChar(char c);
bool CheckKeywords(Reader *r);
void SkipSpace(Reader *r);
void SkipSpaceAndNewlines(Reader *r);
bool Check(Reader *r, char *expect);
bool Match(Reader *r, char *expect);
void Expect(Reader *r, char *expect);
char *PrecName(Precedence prec);

Val ParseBlock(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing block\n");

  SkipSpaceAndNewlines(r);

  Val exps = nil;
  while (!IsEnd(Peek(r))) {
    Val exp = ParsePipe(r);
    if (r->status != PARSE_OK) return nil;
    if (IsNil(exp)) break;

    exps = MakePair(exp, exps);
    SkipSpaceAndNewlines(r);
  }

  return Reverse(exps);
}

Val ParsePipe(Reader *r)
{
  Val exp = ParseExpr(r);
  if (r->status != PARSE_OK) return nil;

  SkipSpaceAndNewlines(r);
  while (Match(r, "|>")) {
    if (DEBUG_PARSE) {
      fprintf(stderr, "Parsing pipe\n");
      PrintSourceContext(r, 0);
    }

    SkipSpace(r);
    Val next = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    exp = MakeTagged(3, "|>", exp, next);

    SkipSpaceAndNewlines(r);

    if (DEBUG_PARSE) {
      fprintf(stderr, "Produced ");
      PrintVal(exp);
    }
  }

  return exp;
}

Val ParseExpr(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing expr\n");

  Val exp = nil;
  while (!IsEnd(Peek(r))) {
    Val item = ParseInfix(r, PREC_EXPR + 1);
    if (r->status != PARSE_OK) return nil;
    if (IsNil(item)) break;

    exp = MakePair(item, exp);

    SkipSpace(r);
  }

  if (ListLength(exp) == 1) {
    return Head(exp);
  }

  return Reverse(exp);
}

Val ParseDoBlock(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing do-block\n");

  SkipSpaceAndNewlines(r);

  Val exps = MakePair(MakeSymbol("do"), nil);
  while (!Match(r, "end")) {
    if (IsEnd(Peek(r))) return Stop(r);

    if (Match(r, "else")) {
      Val else_block = ParseElseBlock(r);
      if (r->status != PARSE_OK) return nil;

      return MakeTagged(3, "else", Reverse(exps), else_block);
    }

    Val exp = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    exps = MakePair(exp, exps);
    SkipSpaceAndNewlines(r);
  }

  return Reverse(exps);
}

Val ParseElseBlock(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing else-block\n");

  SkipSpaceAndNewlines(r);

  Val exps = MakePair(MakeSymbol("do"), nil);
  while (!Match(r, "end")) {
    if (IsEnd(Peek(r))) return Stop(r);

    Val exp = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    exps = MakePair(exp, exps);
    SkipSpaceAndNewlines(r);
  }

  return Reverse(exps);
}

Val ParseClauses(Reader *r)
{
  if (IsEnd(Peek(r))) return Stop(r);
  if (Match(r, "end")) return nil;
  if (Match(r, "else")) return ParseElseBlock(r);

  Val clause = ParseExpr(r);
  if (r->status != PARSE_OK) return nil;

  if (!IsTagged(clause, "->")) {
    return ParseError(r, "Only clauses allowed in cond");
  }

  SkipSpaceAndNewlines(r);

  Val condition = Second(clause);
  Val consequent = Third(clause);
  Val alternative = ParseClauses(r);
  if (r->status != PARSE_OK) return nil;

  return MakeTagged(3, "if", condition, MakeTagged(3, "else", consequent, alternative));
}

Val ParseCond(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing cond\n");

  SkipSpace(r);
  if (!Match(r, "do")) {
    return ParseError(r, "Expected \"do\" after \"cond\"");
  }

  SkipSpaceAndNewlines(r);

  return ParseClauses(r);
}

Val ParseInfix(Reader *r, Precedence prec)
{
  if (IsEnd(Peek(r))) return Stop(r);

  Val exp;
  PrefixRule *prefix_rule = MatchPrefix(r);

  if (DEBUG_PARSE) {
    if (prefix_rule) {
      fprintf(stderr, "Matched rule for \"%s\" at %s\n", prefix_rule->intro, PrecName(prec));
    } else if (IsDigit(Peek(r))) {
      fprintf(stderr, "Matched number\n");
    } else if (IsSymChar(Peek(r))) {
      fprintf(stderr, "Matched symbol\n");
    }

    PrintSourceContext(r, 0);
  }

  if (prefix_rule) {
    exp = prefix_rule->parse(r);
  } else if (IsDigit(Peek(r))) {
    exp = ParseNumber(r);
  } else if (IsSymChar(Peek(r))) {
    exp = ParseIdentifier(r);
  } else {
    // end of expression reached
    return nil;
  }

  if (r->status != PARSE_OK) return nil;

  if (DEBUG_PARSE) {
    fprintf(stderr, "Produced ");
    PrintVal(exp);
  }

  Operator *op;
  while ((op = MatchOperator(r, prec))) {
    if (DEBUG_PARSE) {
      fprintf(stderr, "Parsing operator \"%s\" at %s\n", op->name, PrecName(prec));
      PrintSourceContext(r, 0);
    }

    if (op->parse) {
      exp = op->parse(r, exp);
      if (r->status != PARSE_OK) return nil;
    } else {
      SkipSpace(r);
      Val operand = ParseInfix(r, op->prec + 1);
      if (r->status != PARSE_OK) return nil;
      exp = MakeList(3, MakeSymbol(op->name), exp, operand);
    }

    if (DEBUG_PARSE) {
      fprintf(stderr, "Produced ");
      PrintVal(exp);
    }
  }

  return exp;
}

PrefixRule *MatchPrefix(Reader *r)
{
  for (u32 i = 0; i < ArrayCount(prefix_rules); i++) {
    if (Match(r, prefix_rules[i].intro)) {
      return &prefix_rules[i];
    }
  }

  return NULL;
}

Operator *MatchOperator(Reader *r, Precedence prec)
{
  for (u32 i = 0; i < ArrayCount(ops); i++) {
    if (Check(r, ops[i].name)) {
      if (ops[i].prec >= prec) {
        Match(r, ops[i].name);
        return &ops[i];
      } else {
        return NULL;
      }
    }
  }

  return NULL;
}

Val ParseLookup(Reader *r, Val prefix)
{
  Val operand = ParseIdentifier(r);
  if (r->status != PARSE_OK) return nil;
  return MakeList(2, prefix, MakeTagged(2, "quote", operand));
}

Val ParseLambda(Reader *r, Val params)
{
  SkipSpace(r);
  Val body = ParseExpr(r);
  if (r->status != PARSE_OK) return nil;
  return MakeTagged(3, "->", params, body);
}

Val ParseNegative(Reader *r)
{
  Val operand = ParseInfix(r, PREC_NEGATIVE);
  return MakeTagged(2, "-", operand);
}

Val ParseGroup(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing group\n");

  SkipSpaceAndNewlines(r);

  Val exp = nil;
  while (!IsEnd(Peek(r))) {
    Val item = ParseInfix(r, PREC_EXPR + 1);
    if (r->status != PARSE_OK) return nil;
    if (IsNil(item)) break;

    exp = MakePair(item, exp);

    SkipSpaceAndNewlines(r);
  }

  if (!Match(r, ")")) return Stop(r);

  if (ListLength(exp) == 1) {
    return Head(exp);
  } else {
    return Reverse(exp);
  }
}

Val ParseList(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing list\n");

  SkipSpaceAndNewlines(r);
  Val list = MakePair(MakeSymbol("list"), nil);

  while (!Match(r, "]")) {
    if (IsEnd(Peek(r))) return Stop(r);

    Val item = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    list = MakePair(item, list);

    Match(r, ",");
    SkipSpaceAndNewlines(r);
  }

  return Reverse(list);
}

Val ParseTuple(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing tuple\n");

  SkipSpaceAndNewlines(r);
  Val list = MakePair(MakeSymbol("tuple"), nil);

  while (!Match(r, "]")) {
    if (IsEnd(Peek(r))) return Stop(r);

    Val item = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    list = MakePair(item, list);

    Match(r, ",");
    SkipSpaceAndNewlines(r);
  }

  return Reverse(list);
}

Val ParseDict(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing dict\n");

  SkipSpaceAndNewlines(r);

  Val keys = nil;
  Val vals = nil;

  while (!Match(r, "}")) {
    if (IsEnd(Peek(r))) return Stop(r);

    Val key = (Match(r, "\"")) ? ParseString(r) : ParseIdentifier(r);
    if (r->status != PARSE_OK) return nil;

    keys = MakePair(key, keys);

    SkipSpace(r);
    if (!Match(r, ":")) {
      return ParseError(r, "Expected \":\"");
    }
    SkipSpace(r);

    Val val = ParseExpr(r);
    if (r->status != PARSE_OK) return nil;

    vals = MakePair(val, vals);
    SkipSpace(r);

    Match(r, ",");
    SkipSpaceAndNewlines(r);
  }

  return MakeTagged(3, "dict", keys, vals);
}

Val ParseNumber(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing number\n");

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
      Retreat(r);
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
    return NumVal(n + frac);
  }
  return IntVal(n);
}

Val ParseString(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing string\n");

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

  return str;
}

Val ParseIdentifier(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing identifier\n");

  if (CheckKeywords(r)) return nil;

  u32 start = r->cur;
  while (IsSymChar(Peek(r))) {
    Advance(r);
  }

  if (r->cur == start) {
    ParseError(r, "Expected symbol");
    return nil;
  }

  return MakeSymbolFromSlice(&r->src[start], r->cur - start);
}

Val ParseSymbol(Reader *r)
{
  if (DEBUG_PARSE) fprintf(stderr, "Parsing symbol\n");

  Val name = ParseIdentifier(r);
  if (r->status != PARSE_OK) return nil;

  return MakeTagged(2, "quote", name);
}



/*
 * Parse helpers
 */

bool IsSymChar(char c)
{
  if (IsAlpha(c)) return true;
  if (IsSpace(c)) return false;
  if (IsNewline(c)) return false;
  if (IsEnd(c)) return false;

  switch (c) {
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case ':':
  case ';':
  case '.':
  case ',':
    return false;
  default:
    return true;
  }
}

bool CheckKeywords(Reader *r)
{
  char *keywords[] = {"else", "end"};

  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (Check(r, keywords[i])) return true;
  }
  return false;
}

void SkipSpace(Reader *r)
{
  while (IsSpace(Peek(r))) {
    Advance(r);
  }

  if (Peek(r) == ';') {
    while (!IsNewline(Peek(r))) {
      Advance(r);
    }
    AdvanceLine(r);
    SkipSpace(r);
  }
}

void SkipSpaceAndNewlines(Reader *r)
{
  SkipSpace(r);
  while (IsNewline(Peek(r))) {
    AdvanceLine(r);
    SkipSpace(r);
  }
}

bool Check(Reader *r, char *expect)
{
  SkipSpace(r);
  u32 len = strlen(expect);
  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }
  return true;
}

bool Match(Reader *r, char *expect)
{
  if (Check(r, expect)) {
    r->cur += strlen(expect);
    r->col += strlen(expect);
    SkipSpace(r);
    return true;
  } else {
    return false;
  }
}

void Expect(Reader *r, char *expect)
{
  if (!Match(r, expect)) {
    u32 len = snprintf(NULL, 0, "Expected \"%s\"", expect);
    char msg[len+1];
    sprintf(msg, "Expected \"%s\"", expect);
    ParseError(r, msg);
  }
}

char *PrecName(Precedence prec)
{
  switch (prec) {
  case PREC_EXPR:     return "PREC_EXPR";
  case PREC_LAMBDA:   return "PREC_LAMBDA";
  case PREC_LOGIC:    return "PREC_LOGIC";
  case PREC_EQUALITY: return "PREC_EQUALITY";
  case PREC_COMPARE:  return "PREC_COMPARE";
  case PREC_RANGE:    return "PREC_RANGE";
  case PREC_TERM:     return "PREC_TERM";
  case PREC_FACTOR:   return "PREC_FACTOR";
  case PREC_EXPONENT: return "PREC_EXPONENT";
  case PREC_CONS:     return "PREC_CONS";
  case PREC_NEGATIVE: return "PREC_NEGATIVE";
  case PREC_LOOKUP:   return "PREC_LOOKUP";
  case PREC_PRIMARY:  return "PREC_PRIMARY";
  default:            return "PREC_UNKNOWN";
  }
}
