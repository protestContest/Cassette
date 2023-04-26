#include "lex.h"
#include "parse_syms.h"
#include "mem.h"

#define LexPeek(lexer, n) (lexer)->src.data[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))
#define IsUppercase(c)    ((c) >= 'A' && (c) <= 'Z')
#define IsLowercase(c)    ((c) >= 'a' && (c) <= 'z')
#define IsAlpha(c)        (IsUpper(c) || IsLower(c))

static bool IsIDChar(char c);
static void SkipWhitespace(Lexer *lexer);
static bool Match(Lexer *lexer, char *expected);
static bool MatchKeyword(Lexer *lexer, char *expected);
static Token MakeToken(u32 type, Lexer *lexer, Val value);
static u32 ParseDigit(char c);
static Val ParseInt(char *src, u32 length, u32 base);
static Val ParseFloat(char *src, u32 length);
static Token NumberToken(Lexer *lexer);
static Token StringToken(Lexer *lexer);
static Token IDToken(Lexer *lexer);
static Token NewlineToken(Lexer *lexer);

void InitLexer(Lexer *lexer, u32 num_literals, Literal *literals, Source src, Mem *mem)
{
  for (u32 i = 0; i < num_literals; i++) {
    MakeSymbol(mem, literals[i].lexeme);
  }
  *lexer = (Lexer){src, 0, 0, 1, 1, mem, num_literals, literals};
}

Token NextToken(Lexer *lexer)
{
  SkipWhitespace(lexer);
  lexer->start = lexer->pos;

  char c = LexPeek(lexer, 0);

  if (c == '\0') return MakeToken(ParseSymEOF, lexer, MakeSymbol(lexer->mem, "$"));
  if (IsDigit(c) || (c == '-' && IsDigit(LexPeek(lexer, 1)))) return NumberToken(lexer);
  if (Match(lexer, "\"")) return StringToken(lexer);
  if (Match(lexer, "\n")) return NewlineToken(lexer);

  for (u32 i = 0; i < lexer->num_literals; i++) {
    char *lexeme = lexer->literals[i].lexeme;
    if (MatchKeyword(lexer, lexeme)) {
      return MakeToken(lexer->literals[i].symbol, lexer, MakeSymbol(lexer->mem, lexeme));
    } else if (Match(lexer, lexeme)) {
      return MakeToken(lexer->literals[i].symbol, lexer, MakeSymbol(lexer->mem, lexeme));
    }
  }

  return IDToken(lexer);
}

void PrintToken(Token token)
{
  for (u32 i = 0; i < token.length; i++) {
    if (IsNewline(token.lexeme[i])) Print("⏎");
    else PrintChar(token.lexeme[i]);
  }
}

static Token MakeToken(u32 type, Lexer *lexer, Val value)
{
  Token token = (Token){
    type,
    lexer->line,
    lexer->col,
    lexer->src.data + lexer->start,
    lexer->pos - lexer->start,
    value
  };
  return token;
}

static Token NumberToken(Lexer *lexer)
{
  bool neg = Match(lexer, "-");

  if (Match(lexer, "0x") && IsHexDigit(LexPeek(lexer, 2))) {
    while (IsHexDigit(LexPeek(lexer, 0))) lexer->pos++;
    Val value = ParseInt(lexer->src.data + lexer->start + 2, lexer->pos - lexer->start + 2, 16);
    if (neg) value = IntVal(-RawInt(value));
    return MakeToken(ParseSymNUM, lexer, value);
  }

  while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;

  Val value;
  if (LexPeek(lexer, 0) == '.' && IsDigit(LexPeek(lexer, 1))) {
    lexer->pos++;
    while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;
    value = ParseFloat(lexer->src.data + lexer->start, lexer->pos - lexer->start);
    if (neg) value = NumVal(-RawNum(value));
  } else {
    value = ParseInt(lexer->src.data + lexer->start, lexer->pos - lexer->start, 10);
    if (neg) value = IntVal(-RawInt(value));
  }

  return MakeToken(ParseSymNUM, lexer, value);
}

static Val ParseInt(char *src, u32 length, u32 base)
{
  i32 num = 0;
  for (u32 i = 0; i < length; i++) {
    num *= base;
    u32 digit = ParseDigit(src[i]);
    num += digit;
  }

  return IntVal(num);
}

static Val ParseFloat(char *src, u32 length)
{
  u32 whole = 0;

  for (u32 i = 0; i < length; i++) {
    if (src[i] == '.') {
      float frac = 0;
      float place = 0.1;
      for (u32 j = i + 1; j < length; j++) {
        u32 digit = ParseDigit(src[j]);
        frac += digit * place;
        place /= 10;
      }

      return NumVal((float)whole + frac);
    }

    whole *= 10;
    u32 digit = ParseDigit(src[i]);
    whole += digit;
  }
  return NumVal((float)whole);
}

static u32 ParseDigit(char c)
{
  if (IsDigit(c)) return c - '0';
  if (IsUppercase(c)) return c - 'A';
  if (IsLowercase(c)) return c - 'a';
  return 0;
}

static Token StringToken(Lexer *lexer)
{
  Match(lexer, "\"");
  while (LexPeek(lexer, 0) != '"') {
    if (LexPeek(lexer, 0) == '\\') lexer->pos++;
    if (LexPeek(lexer, 0) == '\0') break;
    lexer->pos++;
  }
  Match(lexer, "\"");
  char *lexeme = lexer->src.data + lexer->start + 1;
  u32 length = lexer->pos - lexer->start - 2;
  Val binary = MakeBinaryFrom(lexer->mem, lexeme, length);

  return MakeToken(ParseSymSTR, lexer, binary);
}

static Token IDToken(Lexer *lexer)
{
  while (LexPeek(lexer, 0) != '\0' && IsIDChar(LexPeek(lexer, 0))) lexer->pos++;
  Val value = MakeSymbolFrom(lexer->mem, lexer->src.data + lexer->start, lexer->pos - lexer->start);
  return MakeToken(ParseSymID, lexer, value);
}

static Token NewlineToken(Lexer *lexer)
{
  lexer->line++;
  lexer->col = 1;
  while (Match(lexer, "\n")) {
    lexer->line++;
    SkipWhitespace(lexer);
  }
  return MakeToken(ParseSymNL, lexer, nil);
}

static bool Match(Lexer *lexer, char *expected)
{
  u32 i = 0;
  char *c = expected;
  u32 lines = 0;
  u32 col = lexer->col;
  while (*c != '\0') {
    if (LexPeek(lexer, i) != *c) return false;
    i++;
    c++;
    if (IsNewline(*c)) {
      col = 1;
      lines++;
    } else {
      col++;
    }
  }
  lexer->pos += i;
  lexer->line += lines;
  lexer->col = col;
  return true;
}

static bool MatchKeyword(Lexer *lexer, char *expected)
{
  if (!IsIDChar(expected[0])) return false;

  u32 keyword_length = StrLen(expected);
  if (IsIDChar(LexPeek(lexer, keyword_length))) return false;

  return Match(lexer, expected);
}

static bool IsIDChar(char c)
{
  if (IsWhitespace(c)) return false;
  if (IsNewline(c)) return false;

  switch (c) {
  case '(':   return false;
  case ')':   return false;
  case '[':   return false;
  case ']':   return false;
  case '{':   return false;
  case '}':   return false;
  case '.':   return false;
  case '*':   return false;
  case '/':   return false;
  case '+':   return false;
  case '|':   return false;
  case '>':   return false;
  case '<':   return false;
  case '=':   return false;
  case ',':   return false;
  case ':':   return false;
  default:    return true;
  }
}

static void SkipWhitespace(Lexer *lexer)
{
  while (IsWhitespace(LexPeek(lexer, 0))) {
    lexer->pos++;
    lexer->col++;
  }

  if (LexPeek(lexer, 0) == ';') {
    while (!IsNewline(LexPeek(lexer, 0))) {
      if (LexPeek(lexer, 0) == '\0') return;
      lexer->pos++;
    }
    lexer->pos++;
    lexer->col = 1;
    lexer->line++;
    SkipWhitespace(lexer);
  }
}
