#include "reader.h"
#include "mem.h"
#include "value.h"
#include "printer.h"

#define DEBUG_READ 1

/*
 * Source helpers
 */

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define Peek(r)           ((r)->src[(r)->cur])

static bool IsSymChar(char c)
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

/*
 * Reader manipulation
 */

Reader *NewReader(void)
{
  Reader *r = malloc(sizeof(Reader));
  r->src = NULL;
  r->last_ok = NULL;
  r->status = PARSE_OK;
  r->cur = 0;
  r->file = "stdin";
  r->line = 1;
  r->col = 1;
  r->error = NULL;
  r->ast = nil;
  return r;
}

void FreeReader(Reader *r)
{
  free(r->src);
  free(r);
}

void AppendSource(Reader *r, char *src)
{
  if (r->src == NULL) {
    r->src = malloc(strlen(src) + 1);
    strcpy(r->src, src);
  } else {
    r->src = realloc(r->src, strlen(r->src) + strlen(src) + 1);
    strcat(r->src, src);
  }

  if (ReadOk(r)) {
    r->last_ok = r->src;
  }
}

static Val Stop(Reader *r)
{
  r->status = PARSE_INCOMPLETE;
  return nil;
}

static void Rewind(Reader *r)
{
  r->cur = 0;
  r->col = 1;
  r->line = 1;
  r->status = PARSE_OK;
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

static void SkipSpaceAndNewlines(Reader *r)
{
  SkipSpace(r);
  while (IsNewline(Peek(r))) {
    AdvanceLine(r);
    SkipSpace(r);
  }
}

static bool Check(Reader *r, char *expect)
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
  if (Check(r, expect)) {
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
    ParseError(r, "Expected \"%s\"", expect);
  }
}

/*
 * Debugging
 */

void PrintSource(Reader *r)
{
  u32 linenum_size = (r->line >= 1000) ? 4 : (r->line >= 100) ? 3 : (r->line >= 10) ? 2 : 1;
  u32 colnum_size = (r->col >= 1000) ? 4 : (r->col >= 100) ? 3 : (r->col >= 10) ? 2 : 1;
  u32 gutter = (linenum_size > colnum_size + 1) ? linenum_size : colnum_size + 1;
  if (gutter < 3) gutter = 3;

  char *c = r->src;
  u32 line = 1;

  while (!IsEnd(*c)) {
    fprintf(stderr, "  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      fprintf(stderr, "%c", *c);
      c++;
    }

    fprintf(stderr, "\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > r->src && !IsNewline(*(c-1))) {
    fprintf(stderr, "\n");
  }
}

void PrintSourceContext(Reader *r, u32 num_lines)
{
  char *c = &r->src[r->cur];
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    c++;
    after++;
  }
  u32 before = num_lines - after;
  if (before > r->line) {
    before = r->line;
    after = num_lines - r->line;
  }

  u32 gutter = (r->line >= 1000) ? 4 : (r->line >= 100) ? 3 : (r->line >= 10) ? 2 : 1;
  if (gutter < 2) gutter = 2;

  c = r->src;
  i32 line = 1;

  while (line < (i32)r->line - (i32)before) {
    if (IsEnd(*c)) return;
    if (IsNewline(*c)) line++;
    c++;
  }

  while (line < (i32)r->line + (i32)after + 1 && !IsEnd(*c)) {
    fprintf(stderr, "  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      fprintf(stderr, "%c", *c);
      c++;
    }

    if (line == (i32)r->line) {
      fprintf(stderr, "\n  ");
      for (u32 i = 0; i < gutter + r->col + 2; i++) fprintf(stderr, " ");
      fprintf(stderr, "↑");
    }
    fprintf(stderr, "\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > r->src && !IsNewline(*(c-1))) {
    fprintf(stderr, "\n");
  }
}

void PrintReaderError(Reader *r)
{
  if (r->status != PARSE_ERROR) return;

  // red text
  fprintf(stderr, "\x1B[31m");
  fprintf(stderr, "[%s:%d:%d] Parse error: %s\n\n", r->file, r->line, r->col, r->error);

  PrintSourceContext(r, 10);

  // reset text
  fprintf(stderr, "\x1B[0m");
}

/*
 * Parsing
 */

typedef enum {
  PREC_NONE,
  PREC_BLOCK,
  PREC_EXPR,
  PREC_LAMBDA,
  PREC_CALL,
  PREC_LOGIC,
  PREC_EQUALITY,
  PREC_COMPARE,
  PREC_TERM,
  PREC_FACTOR,
  PREC_EXPONENT,
  PREC_NEGATIVE,
  PREC_ACCESS,
  PREC_PRIMARY,
} Precedence;

char *PrecName(Precedence prec)
{
  switch (prec) {
  case PREC_NONE:     return "PREC_NONE";
  case PREC_BLOCK:    return "PREC_BLOCK";
  case PREC_EXPR:     return "PREC_EXPR";
  case PREC_LAMBDA:   return "PREC_LAMBDA";
  case PREC_CALL:     return "PREC_CALL";
  case PREC_LOGIC:    return "PREC_LOGIC";
  case PREC_EQUALITY: return "PREC_EQUALITY";
  case PREC_COMPARE:  return "PREC_COMPARE";
  case PREC_TERM:     return "PREC_TERM";
  case PREC_FACTOR:   return "PREC_FACTOR";
  case PREC_EXPONENT: return "PREC_EXPONENT";
  case PREC_NEGATIVE: return "PREC_NEGATIVE";
  case PREC_ACCESS:   return "PREC_ACCESS";
  case PREC_PRIMARY:  return "PREC_PRIMARY";
  default:            return "PREC_UNKNOWN";
  }
}

struct InfixRule;
typedef Val (*PrefixFn)(Reader *r);
typedef Val (*InfixFn)(Reader *r, struct InfixRule *rule, Val prefix);

typedef struct {
  char *intro;
  PrefixFn parse;
} PrefixRule;

typedef struct InfixRule {
  char *op;
  InfixFn parse;
  Precedence prec;
} InfixRule;

typedef enum {
  DIGIT
} TokenType;

static Val ParsePrec(Reader *r, Precedence prec);

static void PrintPrefixRule(Reader *r, PrefixRule *rule, Precedence prec)
{
  fprintf(stderr, "Prefix rule for \"%s\" at %s\n", rule->intro, PrecName(prec));
  PrintSourceContext(r, 0);
}

static void PrintInfixRule(Reader *r, InfixRule *rule, Precedence prec)
{
  fprintf(stderr, "Infix rule for \"%s\" (%s) at %s\n", rule->op, PrecName(rule->prec), PrecName(prec));
  PrintSourceContext(r, 0);
}

static Val ParseExpr(Reader *r)
{
  Val exp = nil;
  while (!IsEnd(Peek(r))) {
    exp = MakePair(ParsePrec(r, PREC_LAMBDA), exp);
    if (!ReadOk(r)) return nil;
    if (IsNil(Head(exp))) return Reverse(Tail(exp));
    SkipSpace(r);
  }

  return Reverse(exp);
}

static Val ParseBlock(Reader *r)
{
  Val exprs = nil;
  SkipSpaceAndNewlines(r);
  while (!IsEnd(Peek(r))) {
    Val exp = ParseExpr(r);

    if (!ReadOk(r)) return nil;
    if (IsNil(exp)) break;

    exprs = MakePair(exp, exprs);
    SkipSpaceAndNewlines(r);
    while (Peek(r) == ',') {
      Advance(r);
      SkipSpaceAndNewlines(r);
    }
  }

  return Reverse(exprs);
}

static Val ParseDoBlock(Reader *r)
{
  Advance(r);
  Advance(r);

  Val exp = MakePair(MakeSymbol("do"), ParseBlock(r));

  if (Match(r, "else")) {
    Val else_exp = MakePair(MakeSymbol("do"), ParseBlock(r));

    if (!Match(r, "end")) {
      return Stop(r);
    }

    return MakeTagged(3, "ifelse", exp, else_exp);
  }

  if (!Match(r, "end")) {
    return Stop(r);
  }

  return exp;
}

static Val ParseGroup(Reader *r)
{
  Advance(r);
  Val exp = ParseBlock(r);
  if (!Match(r, ")")) {
    return Stop(r);
  }

  if (ListLength(exp) == 1) {
    return Head(exp);
  } else {
    return exp;
  }
}

static Val ParseNumber(Reader *r)
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

static bool CheckKeywords(Reader *r)
{
  char *keywords[] = {"else", "end"};

  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (Check(r, keywords[i])) return true;
  }
  return false;
}

static Val ParseIdentifier(Reader *r)
{
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

static Val ParseNegative(Reader *r)
{
  Advance(r);
  Val operand = ParsePrec(r, PREC_NEGATIVE);
  return MakeTagged(2, "-", operand);
}

static Val ParseString(Reader *r)
{
  Advance(r);
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

static Val ParseSymbol(Reader *r)
{
  Advance(r);
  Val name = ParseIdentifier(r);
  if (!ReadOk(r)) return nil;

  return name;
}

static Val ParseAccess(Reader *r, InfixRule *rule, Val prefix)
{
  Advance(r);
  Val operand = ParseIdentifier(r);
  return MakeTagged(3, ".", prefix, operand);
}

static Val ParseInfix(Reader *r, InfixRule *rule, Val prefix)
{
  for (u32 i = 0; i < strlen(rule->op); i++) Advance(r);
  SkipSpace(r);
  Val operand = ParsePrec(r, rule->prec + 1);
  return MakeTagged(3, rule->op, prefix, operand);
}

PrefixRule num_rule = { "DIGIT", &ParseNumber };
PrefixRule id_rule = { "ALPHA", &ParseIdentifier };

static PrefixRule prefix_rules[] = {
  { "do",   &ParseDoBlock },
  { "-",    &ParseNegative },
  { "\"",   &ParseString },
  { ":",    &ParseSymbol },
  { "(",    &ParseGroup },
};

static InfixRule infix_rules[] = {
  { "and",  &ParseInfix,    PREC_LOGIC },
  { "or",   &ParseInfix,    PREC_LOGIC },
  { "**",   &ParseInfix,    PREC_EXPONENT },
  { ">=",   &ParseInfix,    PREC_COMPARE },
  { "<=",   &ParseInfix,    PREC_COMPARE },
  { "!=",   &ParseInfix,    PREC_EQUALITY },
  { "->",   &ParseInfix,    PREC_LAMBDA },
  { ".",    &ParseAccess,   PREC_ACCESS },
  { "*",    &ParseInfix,    PREC_FACTOR },
  { "/",    &ParseInfix,    PREC_FACTOR },
  { "+",    &ParseInfix,    PREC_TERM },
  { "-",    &ParseInfix,    PREC_TERM },
  { ">",    &ParseInfix,    PREC_COMPARE },
  { "<",    &ParseInfix,    PREC_COMPARE },
  { "=",    &ParseInfix,    PREC_EQUALITY },
};

static PrefixRule *GetPrefixRule(Reader *r)
{
  if (IsDigit(Peek(r)))   return &num_rule;

  for (u32 i = 0; i < ArrayCount(prefix_rules); i++) {
    if (Check(r, prefix_rules[i].intro)) {
      return &prefix_rules[i];
    }
  }

  if (IsSymChar(Peek(r))) return &id_rule;

  return NULL;
}

static InfixRule *GetInfixRule(Reader *r, Precedence prec)
{
  for (u32 i = 0; i < ArrayCount(infix_rules); i++) {
    if (infix_rules[i].prec >= prec && Check(r, infix_rules[i].op)) {
      return &infix_rules[i];
    }
  }
  return NULL;
}

static Val ParsePrec(Reader *r, Precedence prec)
{
  if (IsEnd(Peek(r))) return Stop(r);

  PrefixRule *prefix = GetPrefixRule(r);
  if (!prefix) {
    return nil;
  }
  if (DEBUG_READ) PrintPrefixRule(r, prefix, prec);

  Val exp = prefix->parse(r);
  if (!ReadOk(r)) return nil;

  if (DEBUG_READ) {
    fprintf(stderr, "Produced ");
    PrintVal(exp);
  }

  InfixRule *infix;
  while ((infix = GetInfixRule(r, prec))) {
    if (DEBUG_READ) PrintInfixRule(r, infix, prec);
    exp = infix->parse(r, infix, exp);
    if (!ReadOk(r)) return nil;

    if (DEBUG_READ) {
      fprintf(stderr, "Produced ");
      PrintVal(exp);
    }
  }

  return exp;
}

/* Read functions */

void Read(Reader *r, char *src)
{
  AppendSource(r, src);
  Rewind(r);
  SkipSpaceAndNewlines(r);

  if (DEBUG_READ) {
    PrintSource(r);
    fprintf(stderr, "---\n");
  }

  Val exp = ParseBlock(r);
  if (ReadOk(r)) {
    r->ast = exp;
  }
}

void ReadFile(Reader *reader, char *path)
{
  FILE *file = fopen(path, "r");
  if (!file) {
    ParseError(reader, "Could not open file \"%s\"", path);
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

  reader->file = path;
  Read(reader, src);
}
