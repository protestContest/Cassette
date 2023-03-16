#include "scan.h"
#include <univ/io.h>
#include <univ/str.h>

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define IsBoundary(c)     (IsSpace(c) || IsNewline(c) || IsEnd(c))
static bool IsSymChar(char c);

#define LookAhead(s, i)   ((s)->data[(s)->cur + (i)])
#define Peek(s)           LookAhead(s, 0)
#define PeekNext(s)       LookAhead(s, 1)

static void Advance(Source *source);
static void SkipSpace(Source *source);
static bool Check(Source *source, char *expect);
static bool Match(Source *source, char *expect);
static bool CheckKeyword(Source *source, char *expect);
static bool MatchKeyword(Source *source, char *expect);

static Token MakeToken(Source *source, TokenType type, u32 length);
static Token ErrorToken(Source *source, char *msg);
static Token NumberToken(Source *source);
static Token StringToken(Source *source);
static Token IdentToken(Source *source);

void InitSource(Source *source, char *data)
{
  source->line = 1;
  source->col = 1;
  source->cur = 0;
  source->data = data;
}

Token ScanToken(Source *source)
{
  SkipSpace(source);

  if (IsEnd(Peek(source))) return MakeToken(source, TOKEN_EOF, 0);

  if (IsDigit(Peek(source)))    return NumberToken(source);
  if (Check(source, "\""))      return StringToken(source);

  if (Match(source, "("))       return MakeToken(source, TOKEN_LPAREN, 1);
  if (Match(source, ")"))       return MakeToken(source, TOKEN_RPAREN, 1);
  if (Match(source, "["))       return MakeToken(source, TOKEN_LBRACKET, 1);
  if (Match(source, "]"))       return MakeToken(source, TOKEN_RBRACKET, 1);
  if (Match(source, "{"))       return MakeToken(source, TOKEN_LBRACE, 1);
  if (Match(source, "}"))       return MakeToken(source, TOKEN_RBRACE, 1);

  if (Match(source, "->"))      return MakeToken(source, TOKEN_ARROW, 1);
  if (Match(source, "."))       return MakeToken(source, TOKEN_DOT, 1);
  if (Match(source, "*"))       return MakeToken(source, TOKEN_STAR, 1);
  if (Match(source, "/"))       return MakeToken(source, TOKEN_SLASH, 1);
  if (Match(source, "-"))       return MakeToken(source, TOKEN_MINUS, 1);
  if (Match(source, "+"))       return MakeToken(source, TOKEN_PLUS, 1);
  if (Match(source, "|"))       return MakeToken(source, TOKEN_BAR, 1);
  if (Match(source, ">"))       return MakeToken(source, TOKEN_GREATER, 1);
  if (Match(source, "<"))       return MakeToken(source, TOKEN_LESS, 1);
  if (Match(source, "="))       return MakeToken(source, TOKEN_EQUAL, 1);
  if (Match(source, ","))       return MakeToken(source, TOKEN_COMMA, 1);
  if (Match(source, ":"))       return MakeToken(source, TOKEN_COLON, 1);
  if (Match(source, "\n"))      return MakeToken(source, TOKEN_NEWLINE, 1);
  if (Match(source, "\r"))      return MakeToken(source, TOKEN_NEWLINE, 1);

  if (IsSymChar(Peek(source)))  return IdentToken(source);

  return ErrorToken(source, "Unexpected character");
}

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
  case '"':
  case ',':
  case '.':
  case ':':
  case ';':
    return false;
  default:
    return true;
  }
}

static void Advance(Source *source)
{
  if (IsNewline(Peek(source))) {
    source->line++;
    source->col = 1;
  } else {
    source->col++;
  }
  source->cur++;
}

static void SkipSpace(Source *source)
{
  while (IsSpace(Peek(source))) {
    Advance(source);
  }

  if (Peek(source) == ';') {
    while (!IsNewline(Peek(source))) {
      Advance(source);
    }
  }
}

static bool Check(Source *source, char *expect)
{
  u32 len = StrLen(expect);

  for (u32 i = 0; i < len; i++) {
    if IsEnd(LookAhead(source, i)) return false;
    if (LookAhead(source, i) != expect[i]) return false;
  }

  return true;
}

static bool Match(Source *source, char *expect)
{
  if (Check(source, expect)) {
    for (u32 i = 0; i < StrLen(expect); i++) {
      Advance(source);
    }
    return true;
  } else {
    return false;
  }
}

static bool MatchKeyword(Source *source, char *expect)
{
  u32 len = StrLen(expect);
  for (u32 i = 0; i < len; i++) {
    if (IsEnd(Peek(source))) return false;
    if (Peek(source) != expect[i]) return false;
    Advance(source);
  }
  return !IsSymChar(Peek(source));
}

static Token MakeToken(Source *source, TokenType type, u32 length)
{
  Token token;
  token.type = type;
  token.line = source->line;
  token.col = source->col;
  token.lexeme = source->data + source->cur - length;
  token.length = length;
  return token;
}

static Token ErrorToken(Source *source, char *msg)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.line = source->line;
  token.col = source->col;
  token.lexeme = msg;
  token.length = StrLen(msg);
  return token;
}

static Token NumberToken(Source *source)
{
  u32 start = source->cur;

  while (IsDigit(Peek(source)) || Peek(source) == '_') Advance(source);
  if (Check(source, ".") && IsDigit(PeekNext(source))) {
    Match(source, ".");
    while (IsDigit(Peek(source)) || Peek(source) == '_') Advance(source);
  }

  return MakeToken(source, TOKEN_NUMBER, source->cur - start);
}

static Token StringToken(Source *source)
{
  u32 start = source->cur;
  Match(source, "\"");
  while (!Check(source, "\"") && !IsEnd(Peek(source))) {
    Advance(source);
  }

  if (IsEnd(Peek(source))) {
    return ErrorToken(source, "Unterminated string");
  }

  Advance(source);
  return MakeToken(source, TOKEN_STRING, source->cur - start);
}

static Token IdentToken(Source *source)
{
  u32 start = source->cur;
  TokenType type = TOKEN_IDENT;

  if (MatchKeyword(source, "and")) {
    type = TOKEN_AND;
  } else if (MatchKeyword(source, "cond")) {
    type = TOKEN_COND;
  } else if (MatchKeyword(source, "d")) {
    if (MatchKeyword(source, "ef")) {
      type = TOKEN_DEF;
    } else if (MatchKeyword(source, "o")) {
      type = TOKEN_DO;
    }
  } else if (MatchKeyword(source, "e")) {
    if (MatchKeyword(source, "lse")) {
      type = TOKEN_ELSE;
    } else if (MatchKeyword(source, "nd")) {
      type = TOKEN_END;
    }
  } else if (MatchKeyword(source, "if")) {
    type = TOKEN_IF;
  } else if (MatchKeyword(source, "not")) {
    type = TOKEN_NOT;
  } else if (MatchKeyword(source, "or")) {
    type = TOKEN_OR;
  }

  while (IsSymChar(Peek(source))) Advance(source);
  return MakeToken(source, type, source->cur - start);
}

char *TokenStr(TokenType type)
{
  switch (type) {
  case TOKEN_EOF:         return "EOF";
  case TOKEN_ERROR:       return "ERROR";
  case TOKEN_NUMBER:      return "NUMBER";
  case TOKEN_STRING:      return "STRING";
  case TOKEN_IDENT:       return "IDENT";
  case TOKEN_LPAREN:      return "LPAREN";
  case TOKEN_RPAREN:      return "RPAREN";
  case TOKEN_LBRACKET:    return "LBRACKET";
  case TOKEN_RBRACKET:    return "RBRACKET";
  case TOKEN_LBRACE:      return "LBRACE";
  case TOKEN_RBRACE:      return "RBRACE";
  case TOKEN_DOT:         return "DOT";
  case TOKEN_STAR:        return "STAR";
  case TOKEN_SLASH:       return "SLASH";
  case TOKEN_MINUS:       return "MINUS";
  case TOKEN_PLUS:        return "PLUS";
  case TOKEN_BAR:         return "BAR";
  case TOKEN_GREATER:     return "GREATER";
  case TOKEN_LESS:        return "LESS";
  case TOKEN_EQUAL:       return "EQUAL";
  case TOKEN_COMMA:       return "COMMA";
  case TOKEN_COLON:       return "COLON";
  case TOKEN_NEWLINE:     return "NEWLINE";
  case TOKEN_ARROW:       return "ARROW";
  case TOKEN_AND:         return "AND";
  case TOKEN_COND:        return "COND";
  case TOKEN_DEF:         return "DEF";
  case TOKEN_DO:          return "DO";
  case TOKEN_ELSE:        return "ELSE";
  case TOKEN_END:         return "END";
  case TOKEN_IF:          return "IF";
  case TOKEN_NOT:         return "NOT";
  case TOKEN_OR:          return "OR";
  default:                return "";
  }
}

void PrintSourceContext(Source *source, Token cur_token, u32 num_lines)
{
  char *c = &Peek(source);
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    if (IsNewline(*c)) after++;
    c++;
  }

  u32 before = num_lines - after;
  if (before > source->line) {
    before = source->line;
    after = num_lines - source->line;
  }

  u32 gutter = (source->line >= 1000) ? 4 : (source->line >= 100) ? 3 : (source->line >= 10) ? 2 : 1;
  if (gutter < 2) gutter = 2;

  c = source->data;
  i32 line = 1;

  while (line < (i32)source->line - (i32)before) {
    if (IsEnd(*c)) return;
    if (IsNewline(*c)) line++;
    c++;
  }

  Print("  ");
  PrintInt(line, gutter);
  Print(" ");
  while (!IsNewline(*c) && !IsEnd(*c)) {
    if (c == cur_token.lexeme) {
      Print("\x1B[4m");
    }
    if (c == cur_token.lexeme + cur_token.length) {
      Print("\x1B[0m");
    }
    AppendByte(output, *c++);
  }
  Print("\x1B[0m");
  Print("\n");
  for (u32 i = 0; i < source->col + gutter + 1; i++) Print(" ");
  Print("↑\n");
  line++;
  c++;

  while (line < (i32)source->line + (i32)after + 1 && !IsEnd(*c)) {
    Print("  ");
    PrintInt(line, gutter);
    Print(" │ ");
    while (!IsNewline(*c) && !IsEnd(*c)) {
      AppendByte(output, *c++);
    }

    Print("\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > source->data && !IsNewline(*(c-1))) {
    Print("\n");
  }
}
