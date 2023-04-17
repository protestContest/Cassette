#include "lex.h"
#include "parse_syms.h"
#include "mem.h"

bool IsIDChar(char c)
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

void SkipWhitespace(Lexer *lexer)
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

bool Match(Lexer *lexer, char *expected)
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

bool MatchKeyword(Lexer *lexer, char *expected)
{
  u32 i = 0;
  char *c = expected;
  while (*c != '\0') {
    if (LexPeek(lexer, i) != *c) return false;
    i++;
    c++;
  }

  if (IsIDChar(LexPeek(lexer, i + 1))) return false;

  lexer->pos += i;
  return true;
}

Token MakeToken(u32 type, Lexer *lexer, Val value)
{
  Token token = (Token){
    type,
    lexer->line,
    lexer->col,
    lexer->src + lexer->start,
    lexer->pos - lexer->start,
    value
  };
  return token;
}

Token ErrorToken(Lexer *lexer, char *msg)
{
  char *c = msg;
  while (*c != '\0') c++;
  Token token = {-1, lexer->line, lexer->col, msg, c - msg, nil};
  return token;
}

static u32 ParseDigit(char c)
{
  if (IsDigit(c)) return c - '0';
  if (IsUppercase(c)) return c - 'A';
  if (IsLowercase(c)) return c - 'a';
  return 0;
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

static Token NumberToken(Lexer *lexer)
{
  if (Match(lexer, "0x") && IsHexDigit(LexPeek(lexer, 2))) {
    while (IsHexDigit(LexPeek(lexer, 0))) lexer->pos++;
    Val value = ParseInt(lexer->src + lexer->start + 2, lexer->pos - lexer->start + 2, 16);
    return MakeToken(ParseSymNUM, lexer, value);
  }

  while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;

  Val value;
  if (LexPeek(lexer, 0) == '.' && IsDigit(LexPeek(lexer, 1))) {
    lexer->pos++;
    while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;
    value = ParseFloat(lexer->src + lexer->start, lexer->pos - lexer->start);
  } else {
    value = ParseInt(lexer->src + lexer->start, lexer->pos - lexer->start, 10);
  }

  return MakeToken(ParseSymNUM, lexer, value);
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
  char *lexeme = lexer->src + lexer->start + 1;
  u32 length = lexer->pos - lexer->start - 2;
  Val binary = MakeBinaryFrom(lexer->mem, lexeme, length);

  return MakeToken(ParseSymSTR, lexer, binary);
}

static Token IDToken(Lexer *lexer)
{
  while (LexPeek(lexer, 0) != '\0' && IsIDChar(LexPeek(lexer, 0))) lexer->pos++;
  Val value = MakeSymbolFrom(lexer->mem, lexer->src + lexer->start, lexer->pos - lexer->start);
  return MakeToken(ParseSymID, lexer, value);
}

static Token NewlinesToken(Lexer *lexer)
{
  lexer->line++;
  lexer->col = 1;
  while (Match(lexer, "\n")) {
    lexer->line++;
    SkipWhitespace(lexer);
  }
  return MakeToken(ParseSymNL, lexer, nil);
}

void InitLexer(Lexer *lexer, u32 num_literals, Literal *literals, char *src, Mem *mem)
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
  if (IsDigit(c)) return NumberToken(lexer);
  if (Match(lexer, "\"")) return StringToken(lexer);
  if (Match(lexer, "\n")) return NewlinesToken(lexer);

  for (u32 i = 0; i < lexer->num_literals; i++) {
    char *lexeme = lexer->literals[i].lexeme;
    if (Match(lexer, lexeme)) {
      return MakeToken(lexer->literals[i].symbol, lexer, MakeSymbol(lexer->mem, lexeme));
    }
  }

  return IDToken(lexer);
}

int PrintToken(Token token)
{
  for (u32 i = 0; i < token.length; i++) {
    if (IsNewline(token.lexeme[i])) Print("⏎");
    else PrintN(token.lexeme + i, 1);
  }
  return token.length;
}

int DebugToken(Token token)
{
  if (token.length > 0) {
    Print("[");
    PrintInt(token.type);
    Print("] ");
    PrintN(token.lexeme, token.length);
    return 0;
  } else {
    Print("[");
    PrintInt(token.type);
    Print("] ");
    return 0;
  }
}

void PrintSourceContext(Lexer *lexer, u32 num_lines)
{
  u32 before = num_lines;
  if (before > lexer->line - 1) before = lexer->line - 1;
  u32 after = num_lines - before;

  u32 start = lexer->line - before;
  u32 end = lexer->line + after;

  char *c = lexer->src;
  u32 line = 1;
  while (*c != '\0' && line < start) {
    if (IsNewline(*c)) line++;
    c++;
  }

  while (*c != '\0' && line <= end) {
    if (line == lexer->line) {
      Print("→");
      PrintIntN(line, 3);
      Print("  ");
    } else {
      PrintIntN(line, 4);
      Print("  ");
    }

    while (*c != '\0' && !IsNewline(*c)) {
      bool underline = c - lexer->src == lexer->start && lexer->start <= lexer->pos;
      if (underline) {
        Print(IOUnderline);
      }
      PrintN(c, 1);
      c++;
      if (underline) {
        Print(IONoUnderline);
      }
    }
    Print("\n");
    c++;

    line++;
  }
}
