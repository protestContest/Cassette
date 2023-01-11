#include "reader.h"
#include "mem.h"
#include "value.h"
#include "printer.h"

#define DEBUG_READ 0

typedef struct {
  char *src;
  u32 cur;
  u32 line;
  u32 col;
} Reader;

typedef enum {
  PREC_NONE,
  PREC_EXPR,
  PREC_BOOLEAN,
  PREC_COMPARE,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_ACCESS,
} Precedence;

typedef enum {
  NONE,
  PAREN,
  BRACKET,
  BRACE,
  QUOTE,
  QUOTE2,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  COLON,
  BLOCK,
  ARROW,
  DOT,
  DIGIT,
  SYMCHAR,
  DEF,
  IF,
  EQUAL,
  INEQUAL,
  LESS,
  LT_EQ,
  GREATER,
  GT_EQ,
  AND,
  OR,
  NOT,
} Sigil;

typedef Val (*PrefixFn)(Reader *r);
typedef Val (*InfixFn)(Reader *r, Val prefix);

typedef struct {
  char *name;
  PrefixFn prefix;
  InfixFn infix;
  Precedence prec;
} ParseRule;

#define IsSpace(c)        ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsUTFChar(c)      (((c) & 0x80) == 0x80)
#define IsEnd(c)          ((c) == '\0')
#define Peek(r)           ((r)->src[(r)->cur])

static void SkipSpace(Reader *r);
static bool WillMatch(Reader *r, char *expect);
static bool Match(Reader *r, char *expected);
static bool IsSymChar(char c);

static Val ParsePrecedence(Reader *r, Precedence prec);

static Val ParseExpr(Reader *r);
static Val ParseInt(Reader *r);
static Val ParseString(Reader *r);
static Val ParseString2(Reader *r);
static Val ParseSymbol(Reader *r);
static Val ParseVariable(Reader *r);
static Val ParseBlock(Reader *r);
static Val ParseList(Reader *r);
static Val ParseListLiteral(Reader *r);
static Val ParseDict(Reader *r);
static Val ParseLambda(Reader *r, Val prefix);
static Val ParseUnary(Reader *r);
static Val ParseNegate(Reader *r);
static Val ParseAdd(Reader *r, Val prefix);
static Val ParseSubtract(Reader *r, Val prefix);
static Val ParseMultiply(Reader *r, Val prefix);
static Val ParseDivide(Reader *r, Val prefix);
static Val ParseDefine(Reader *r);
static Val ParseIf(Reader *r);

static Val ParseAccess(Reader *r, Val prefix);
static Val ParseDotAccess(Reader *r, Val prefix);
static Val ParseCompare(Reader *r, Val prefix);
static Val ParseBool(Reader *r, Val prefix);

static void PrintRule(Reader *r, char *name, bool infix, Precedence prec);
static void PrintReader(Reader *r);
static void PrintReaderTo(FILE *stream, Reader *r);
static Val ParseError(Reader *r, char *message);

ParseRule rules[] = {
  [PAREN] =   { "PAREN",    &ParseList,         NULL,             PREC_NONE },
  [BRACKET] = { "BRACKET",  &ParseListLiteral,  NULL,             PREC_NONE },
  [BRACE] =   { "BRACE",    &ParseDict,         NULL,             PREC_NONE },
  [QUOTE] =   { "QUOTE",    &ParseString,       NULL,             PREC_NONE },
  [QUOTE2] =  { "QUOTE2",   &ParseString2,      NULL,             PREC_NONE },
  [PLUS] =    { "PLUS",     &ParseSymbol,       &ParseAdd,        PREC_TERM },
  [MINUS] =   { "MINUS",    &ParseNegate,       &ParseSubtract,   PREC_TERM },
  [STAR] =    { "STAR",     &ParseSymbol,       &ParseMultiply,   PREC_FACTOR },
  [SLASH] =   { "SLASH",    &ParseSymbol,       &ParseDivide,     PREC_FACTOR },
  [COLON] =   { "COLON",    &ParseSymbol,       NULL,             PREC_NONE },
  [BLOCK] =   { "BLOCK",    &ParseBlock,        NULL,             PREC_NONE },
  [ARROW] =   { "ARROW",    NULL,               &ParseLambda,     PREC_EXPR },
  [DOT] =     { "DOT",      NULL,               &ParseDotAccess,  PREC_ACCESS },
  [DIGIT] =   { "DIGIT",    &ParseInt,          NULL,             PREC_NONE },
  [SYMCHAR] = { "SYMCHAR",  &ParseVariable,     NULL,             PREC_NONE },
  [DEF] =     { "DEF",      &ParseDefine,       NULL,             PREC_NONE },
  [IF] =      { "IF",       &ParseIf,           NULL,             PREC_NONE },
  [EQUAL] =   { "EQUAL",    &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [INEQUAL] = { "INEQUAL",  &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [LESS] =    { "LESS",     &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [LT_EQ] =   { "LT_EQ",    &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [GREATER] = { "GREATER",  &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [GT_EQ] =   { "GT_EQ",    &ParseSymbol,       &ParseCompare,    PREC_COMPARE },
  [AND] =     { "AND",      &ParseSymbol,       &ParseBool,       PREC_BOOLEAN },
  [OR] =      { "OR",       &ParseSymbol,       &ParseBool,       PREC_BOOLEAN },
  [NOT] =     { "NOT",      &ParseSymbol,       &ParseBool,       PREC_BOOLEAN },
};

static ParseRule *GetPrefixRule(Reader *r)
{
  SkipSpace(r);

  if (Match(r, "("))      return &rules[PAREN];
  if (Match(r, "["))      return &rules[BRACKET];
  if (Match(r, "{"))      return &rules[BRACE];
  if (Match(r, "«"))      return &rules[QUOTE];
  if (Match(r, "\""))     return &rules[QUOTE2];
  if (Match(r, ":"))      return &rules[COLON];
  if (Match(r, "do"))     return &rules[BLOCK];
  if (Match(r, "def"))    return &rules[DEF];
  if (Match(r, "if"))     return &rules[IF];
  if (IsDigit(Peek(r)))   return &rules[DIGIT];
  if (IsSymChar(Peek(r))) return &rules[SYMCHAR];
  return NULL;
}

static bool InfixApplies(Sigil type, Precedence prec)
{
  return rules[type].infix != NULL && rules[type].prec >= prec;
}

static ParseRule *GetInfixRule(Reader *r, Precedence prec)
{
  if (InfixApplies(PAREN, prec)   && Match(r, "("))       return &rules[PAREN];
  if (InfixApplies(BRACKET, prec) && Match(r, "["))       return &rules[BRACKET];
  if (InfixApplies(BRACE, prec)   && Match(r, "{"))       return &rules[BRACE];
  if (InfixApplies(QUOTE, prec)   && Match(r, "«"))       return &rules[QUOTE];
  if (InfixApplies(QUOTE2, prec)  && Match(r, "\""))      return &rules[QUOTE2];
  if (InfixApplies(PLUS, prec)    && Match(r, "+"))       return &rules[PLUS];
  if (InfixApplies(MINUS, prec)   && Match(r, "-"))       return &rules[MINUS];
  if (InfixApplies(STAR, prec)    && Match(r, "*"))       return &rules[STAR];
  if (InfixApplies(SLASH, prec)   && Match(r, "/"))       return &rules[SLASH];
  if (InfixApplies(COLON, prec)   && Match(r, ":"))       return &rules[COLON];
  if (InfixApplies(ARROW, prec)   && Match(r, "->"))      return &rules[ARROW];
  if (InfixApplies(DOT, prec)     && Match(r, "."))       return &rules[DOT];
  if (InfixApplies(BLOCK, prec)   && Match(r, "do"))      return &rules[BLOCK];
  if (InfixApplies(DEF, prec)     && Match(r, "def"))     return &rules[DEF];
  if (InfixApplies(IF, prec)      && Match(r, "if"))      return &rules[IF];
  if (InfixApplies(EQUAL, prec)   && WillMatch(r, "="))   return &rules[EQUAL];
  if (InfixApplies(INEQUAL, prec) && WillMatch(r, "≠"))   return &rules[INEQUAL];
  if (InfixApplies(LESS, prec)    && WillMatch(r, "<"))   return &rules[LESS];
  if (InfixApplies(LT_EQ, prec)   && WillMatch(r, "≤"))   return &rules[LT_EQ];
  if (InfixApplies(GREATER, prec) && WillMatch(r, ">"))   return &rules[GREATER];
  if (InfixApplies(GT_EQ, prec)   && WillMatch(r, "≥"))   return &rules[GT_EQ];
  if (InfixApplies(AND, prec)     && WillMatch(r, "and")) return &rules[AND];
  if (InfixApplies(OR, prec)      && WillMatch(r, "or"))  return &rules[OR];
  if (InfixApplies(NOT, prec)     && WillMatch(r, "not")) return &rules[NOT];
  if (InfixApplies(DIGIT, prec)   && IsDigit(Peek(r)))    return &rules[DIGIT];
  if (InfixApplies(SYMCHAR, prec) && IsSymChar(Peek(r)))  return &rules[SYMCHAR];

  return NULL;
}

static Val ParsePrecedence(Reader *r, Precedence prec)
{
  SkipSpace(r);
  if (IsEnd(Peek(r))) ParseError(r, "Unexpected end of text");

  if (DEBUG_READ) PrintReader(r);
  ParseRule *rule = GetPrefixRule(r);
  if (DEBUG_READ) PrintRule(r, rule->name, false, prec);
  if (rule->prefix == NULL) ParseError(r, "Expected expression");

  Val exp = rule->prefix(r);
  while ((rule = GetInfixRule(r, prec))) {
    if (DEBUG_READ) PrintRule(r, rule->name, true, prec);
    exp = rule->infix(r, exp);
  }

  return exp;
}

Val Read(char *src)
{
  Reader r = { src, 0, 1, 1};

  SkipSpace(&r);
  if (IsEnd(Peek(&r))) {
    return nil;
  }

  Val items = nil;
  while (!IsEnd(Peek(&r))) {
    items = MakePair(ParseExpr(&r), items);
  }

  if (IsNil(Tail(items))) {
    return Head(items);
  } else {
    return MakePair(MakeSymbol("do"), Reverse(items));
  }
}

Val ReadFile(char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    Error("Could not open file \"%s\"", path);
  }

  fseek(file, 0, SEEK_END);
  u32 size = ftell(file);
  rewind(file);

  char src[size+1];
  for (u32 i = 0; i < size; i++) {
    int c = fgetc(file);
    src[i] = (char)c;
  }
  src[size] = '\0';

  return Read(src);
}

static void Advance(Reader *r)
{
  r->col++;
  r->cur++;
}

static void AdvanceLine(Reader *r)
{
  r->line++;
  r->col = 1;
  r->cur++;
}

static void SkipSpace(Reader *r)
{
  while (IsSpace(Peek(r))) {
    if (Peek(r) == '\n') {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }

  if (Peek(r) == ';') {
    while (Peek(r) != '\n') {
      Advance(r);
    }
    SkipSpace(r);
  }
}

static bool WillMatch(Reader *r, char *expect)
{
  SkipSpace(r);
  u32 len = strlen(expect);
  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }
  return true;
}

static bool Match(Reader *r, char *expect)
{
  if (WillMatch(r, expect)) {
    r->cur += strlen(expect);
    r->col += strlen(expect);
    SkipSpace(r);
    return true;
  } else {
    return false;
  }
}

static void Expect(Reader *r, char *expect)
{
  if (!Match(r, expect)) {
    char message[strlen(expect) + 12];
    sprintf(message, "Expected \"%s\"", expect);
    ParseError(r, message);
  }
}

static bool IsSymChar(char c)
{
  if (IsAlpha(c)) return true;
  if (IsSpace(c)) return false;

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
    return false;
  default:
    return true;
  }
}

static Val ParseExpr(Reader *r)
{
  return ParsePrecedence(r, PREC_EXPR);
}

static Val ParseInt(Reader *r)
{
  u32 n = Peek(r) - '0';
  Advance(r);
  while (IsDigit(Peek(r))) {
    u32 d = Peek(r) - '0';
    n = n*10 + d;
    Advance(r);
    while (Peek(r) == '_') Advance(r);
  }
  return IntVal(n);
}

static void SkipString(Reader *r)
{
  while (!Match(r, "»")) {
    if (IsEnd(Peek(r))) ParseError(r, "Unterminated string");
    if (Match(r, "«")) {
      SkipString(r);
    }

    if (Peek(r) == '\n') {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }
}

static Val ParseString(Reader *r)
{
  u32 start = r->cur;

  SkipString(r);

  u32 length = r->cur - start;
  return MakeBinary(r->src + start, length);
}

static Val ParseString2(Reader *r)
{
  u32 start = r->cur;

  while (!Match(r, "\"")) {
    if (Peek(r) == '\\') Advance(r);
    if (IsEnd(Peek(r))) return ParseError(r, "Unterminated string");

    if (Peek(r) == '\n') {
      AdvanceLine(r);
    } else {
      Advance(r);
    }
  }

  u32 length = r->cur - start;
  return MakeBinary(r->src + start, length);
}

static Val ParseVariable(Reader *r)
{
  u32 start = r->cur;
  while (IsSymChar(Peek(r))) {
    Advance(r);
  }

  if (r->cur == start) ParseError(r, "Expected symbol");

  return MakeSymbolFromSlice(&r->src[start], r->cur - start);
}

static Val ParseSymbol(Reader *r)
{
  Val var = ParseVariable(r);
  return MakePair(MakeSymbol("quote"), var);
}

static Val ParseBlock(Reader *r)
{
  Val block = MakePair(MakeSymbol("do"), nil);

  while (!Eq(Head(block), SymbolFor("end"))) {
    block = MakePair(ParseExpr(r), block);
  }

  return Reverse(Tail(block));
}

static Val ParseList(Reader *r)
{
  Val items = nil;

  while (!Match(r, ")")) {
    items = MakePair(ParseExpr(r), items);
  }

  return Reverse(items);
}

static Val ParseListLiteral(Reader *r)
{
  Val items = nil;

  while (!Match(r, "]")) {
    if (Match(r, "|")) {
      if (ListLength(items) != 1) {
        ParseError(r, "Can only add one head to a list");
      }
      Val head = Head(items);
      Val tail = ParseExpr(r);
      Expect(r, "]");

      return MakeList(3, MakeSymbol("pair"), head, tail);
    }
    Val exp = ParseExpr(r);
    items = MakePair(exp, items);
  }

  return MakePair(MakeSymbol("list"), Reverse(items));
}

static Val ParseDict(Reader *r)
{
  Val keys = nil;
  Val vals = nil;

  while (!Match(r, "}")) {
    keys = MakePair(ParseSymbol(r), keys);
    Expect(r, ":");
    vals = MakePair(ParseExpr(r), vals);
  }

  return MakeList(3, MakeSymbol("dict"),
                     MakePair(MakeSymbol("list"), keys),
                     MakePair(MakeSymbol("list"), vals));
}

static Val ParseLambda(Reader *r, Val params)
{
  return MakeList(3, MakeSymbol("λ"), params, ParseExpr(r));
}

static Val ParseError(Reader *r, char *message)
{
  u32 len = snprintf(NULL, 0, "[%d:%d] Error: %s\n\n", r->line, r->col, message);
  char msg[len+1];
  snprintf(msg, len, "[%d:%d] Error: %s\n\n", r->line, r->col, message);
  return nil;
}

static Val ParseNegate(Reader *r)
{
  Val operand = ParsePrecedence(r, PREC_UNARY);
  return MakeList(2, MakeSymbol("-"), operand);
}

static Val ParseAdd(Reader *r, Val prefix)
{
  Val operand = ParsePrecedence(r, PREC_TERM);
  return MakeList(3, MakeSymbol("+"), prefix, operand);
}

static Val ParseSubtract(Reader *r, Val prefix)
{
  Val operand = ParsePrecedence(r, PREC_TERM);
  return MakeList(3, MakeSymbol("-"), prefix, operand);
}

static Val ParseMultiply(Reader *r, Val prefix)
{
  Val operand = ParsePrecedence(r, PREC_FACTOR);
  return MakeList(3, MakeSymbol("*"), prefix, operand);
}

static Val ParseDivide(Reader *r, Val prefix)
{
  Val operand = ParsePrecedence(r, PREC_FACTOR);
  return MakeList(3, MakeSymbol("/"), prefix, operand);
}

static Val ParseDefine(Reader *r)
{
  Val var = ParseExpr(r);

  if (IsSym(var)) {
    Val val = ParseExpr(r);
    return MakeList(3, MakeSymbol("def"), var, val);
  } else if (IsList(var)) {
    Val args = Tail(var);
    var = Head(var);
    Val body = ParseExpr(r);
    Val lambda = MakeList(3, MakeSymbol("λ"), args, body);
    return MakeList(3, MakeSymbol("def"), var, lambda);
  }

  return ParseError(r, "Can't define that");
}

static Val ParseIf(Reader *r)
{
  Val test = ParseExpr(r);

  if (Match(r, "do")) {
    Val block = MakePair(MakeSymbol("do"), nil);

    while (!Eq(Head(block), SymbolFor("end")) && !Eq(Head(block), SymbolFor("else"))) {
      block = MakePair(ParseExpr(r), block);
    }

    Val else_block = MakePair(MakeSymbol("do"), nil);
    if (Eq(Head(block), SymbolFor("else"))) {
      while (!Eq(Head(else_block), SymbolFor("end"))) {
        else_block = MakePair(ParseExpr(r), else_block);
      }
      else_block = Reverse(Tail(else_block));
    }

    block = Reverse(Tail(block));
    return MakeList(4, MakeSymbol("if"), test, block, else_block);
  } else {
    Val consequent = ParseExpr(r);
    return MakeList(4, MakeSymbol("if"), test, consequent, nil);
  }
}

static Val ParseAccess(Reader *r, Val prefix)
{
  Val operand = ParseExpr(r);
  Expect(r, "]");
  return MakeList(3, MakeSymbol("get"), prefix, operand);
}

static Val ParseDotAccess(Reader *r, Val prefix)
{
  Val operand = ParseSymbol(r);
  return MakeList(3, MakeSymbol("get"), prefix, operand);
}

static Val ParseCompare(Reader *r, Val prefix)
{
  Val op = ParseVariable(r);
  Val operand = ParsePrecedence(r, PREC_COMPARE + 1);
  return MakeList(3, op, prefix, operand);
}

static Val ParseBool(Reader *r, Val prefix)
{
  Val op = ParseVariable(r);
  Val operand = ParsePrecedence(r, PREC_BOOLEAN + 1);
  return MakeList(3, op, prefix, operand);
}

static void PrintRule(Reader *r, char *name, bool infix, Precedence prec)
{
  printf("[%d:%d] ", r->line, r->col);
  if (infix) {
    printf("Infix rule:  ");
  } else {
    printf("Prefix rule: ");
  }
  printf("%s at ", name);
  switch (prec) {
  case PREC_NONE: printf("PREC_NONE"); break;
  case PREC_EXPR: printf("PREC_EXPR"); break;
  case PREC_TERM: printf("PREC_TERM"); break;
  case PREC_FACTOR: printf("PREC_FACTOR"); break;
  case PREC_UNARY: printf("PREC_UNARY"); break;
  case PREC_ACCESS: printf("PREC_ACCESS"); break;
  case PREC_COMPARE: printf("PREC_COMPARE"); break;
  case PREC_BOOLEAN: printf("PREC_BOOLEAN"); break;
  }
  printf("\n");
}

static void PrintReader(Reader *r)
{
  PrintReaderTo(stdout, r);
}

static void PrintReaderTo(FILE *stream, Reader *r)
{
  char *line = r->src + r->cur - (r->col - 1);

  fprintf(stream, "  ");
  while (*line != '\n' && *line != '\0') {
    fprintf(stream, "%c", *line++);
  }

  fprintf(stream, "\n");
  fprintf(stream, "  ");
  for (u32 i = 0; i < r->col - 1; i++) fprintf(stream, " ");
  fprintf(stream, "^\n");
}
