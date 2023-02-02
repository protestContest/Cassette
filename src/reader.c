#include "reader.h"
#include "mem.h"
#include "printer.h"
#include <stdio.h>

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define IsBoundary(c)     (IsSpace(c) || IsNewline(c) || IsEnd(c))

#define Peek(r)           ((r)->src[(r)->cur])
#define PeekNext(r)   ((r)->src[(r)->cur + 1])

static char Advance(Reader *r)
{
  r->col++;
  return r->src[r->cur++];
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
  case ':':
  case ';':
  case '.':
  case ',':
    return false;
  default:
    return true;
  }
}

static bool Check(Reader *r, const char *expect)
{
  u32 len = strlen(expect);

  for (u32 i = 0; i < len; i++) {
    if IsEnd(r->src[r->cur + i]) return false;
    if (r->src[r->cur + i] != expect[i]) return false;
  }

  return true;
}

static bool Match(Reader *r, const char *expect)
{
  if (Check(r, expect)) {
    r->cur += strlen(expect);
    r->col += strlen(expect);
    return true;
  } else {
    return false;
  }
}

bool CheckKeyword(Reader *r, const char *expect)
{
  if (!Check(r, expect)) return false;
  if (IsSymChar(r->src[r->cur + strlen(expect)])) return false;
  return true;
}

bool MatchKeyword(Reader *r, const char *expect)
{
  if (CheckKeyword(r, expect)) {
    Match(r, expect);
    return true;
  } else {
    return false;
  }
}

static Token MakeToken(Reader *r, TokenType type, u32 length)
{
  Token token;
  token.type = type;
  token.line = r->line;
  token.col = r->col;
  token.lexeme = r->src + r->cur - length;
  token.length = length;
  return token;
}

static Token ErrorToken(Reader *r, char *msg)
{
  Token token;
  token.type = TOKEN_ERROR;
  token.line = r->line;
  token.col = r->col;
  token.lexeme = msg;
  token.length = strlen(msg);
  return token;
}

static Token NumberToken(Reader *r)
{
  u32 start = r->cur;

  while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  if (Check(r, ".") && IsDigit(PeekNext(r))) {
    Match(r, ".");
    while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  }

  return MakeToken(r, TOKEN_NUMBER, r->cur - start);
}

static Token StringToken(Reader *r)
{
  u32 start = r->cur - 1;
  while (Peek(r) != '"' && !IsEnd(Peek(r))) {
    if (Peek(r) == '\n') AdvanceLine(r);
    else Advance(r);
  }

  if (IsEnd(Peek(r))) return ErrorToken(r, "Unterminated string");
  Advance(r);
  return MakeToken(r, TOKEN_STRING, r->cur - start);
}

Token IdentifierToken(Reader *r)
{
  u32 start = r->cur;

  if (MatchKeyword(r, "true"))  return MakeToken(r, TOKEN_TRUE, 4);
  if (MatchKeyword(r, "false")) return MakeToken(r, TOKEN_FALSE, 5);
  if (MatchKeyword(r, "nil"))   return MakeToken(r, TOKEN_NIL, 3);
  if (MatchKeyword(r, "and"))   return MakeToken(r, TOKEN_AND, 3);
  if (MatchKeyword(r, "not"))   return MakeToken(r, TOKEN_NOT, 3);
  if (MatchKeyword(r, "cond"))  return MakeToken(r, TOKEN_COND, 4);
  if (MatchKeyword(r, "def"))   return MakeToken(r, TOKEN_DEF, 3);
  if (MatchKeyword(r, "do"))    return MakeToken(r, TOKEN_DO, 2);
  if (MatchKeyword(r, "else"))  return MakeToken(r, TOKEN_ELSE, 4);
  if (MatchKeyword(r, "end"))   return MakeToken(r, TOKEN_END, 3);
  if (MatchKeyword(r, "or"))    return MakeToken(r, TOKEN_OR, 2);

  while (IsSymChar(Peek(r))) Advance(r);

  if (r->cur == start) return ErrorToken(r, "Expected symbol");

  return MakeToken(r, TOKEN_IDENTIFIER, r->cur - start);
}

Reader *InitReader(Reader *r, char *src)
{
  r->status = Ok;
  r->line = 1;
  r->col = 1;
  r->src = src;
  r->cur = 0;
  r->error = NULL;
  return r;
}

Token ScanToken(Reader *r)
{
  SkipSpace(r);

  if (IsEnd(Peek(r))) return MakeToken(r, TOKEN_EOF, 0);

  if (IsDigit(Peek(r))) return NumberToken(r);
  if (Match(r, "\""))   return StringToken(r);

  if (Match(r, "!=")) return MakeToken(r, TOKEN_NEQ, 2);
  if (Match(r, ">=")) return MakeToken(r, TOKEN_GTE, 2);
  if (Match(r, "**")) return MakeToken(r, TOKEN_EXPONENT, 2);
  if (Match(r, "|>")) return MakeToken(r, TOKEN_PIPE, 2);
  if (Match(r, "->")) return MakeToken(r, TOKEN_ARROW, 2);
  if (Match(r, "<=")) return MakeToken(r, TOKEN_LTE, 2);

  if (Match(r, "("))  return MakeToken(r, TOKEN_LPAREN, 1);
  if (Match(r, ")"))  return MakeToken(r, TOKEN_RPAREN, 1);
  if (Match(r, "["))  return MakeToken(r, TOKEN_LBRACKET, 1);
  if (Match(r, "]"))  return MakeToken(r, TOKEN_RBRACKET, 1);
  if (Match(r, "{"))  return MakeToken(r, TOKEN_LBRACE, 1);
  if (Match(r, "}"))  return MakeToken(r, TOKEN_RBRACE, 1);
  if (Match(r, ","))  return MakeToken(r, TOKEN_COMMA, 1);
  if (Match(r, "."))  return MakeToken(r, TOKEN_DOT, 1);
  if (Match(r, "-"))  return MakeToken(r, TOKEN_MINUS, 1);
  if (Match(r, "+"))  return MakeToken(r, TOKEN_PLUS, 1);
  if (Match(r, "*"))  return MakeToken(r, TOKEN_STAR, 1);
  if (Match(r, "/"))  return MakeToken(r, TOKEN_SLASH, 1);
  if (Match(r, "|"))  return MakeToken(r, TOKEN_BAR, 1);
  if (Match(r, "="))  return MakeToken(r, TOKEN_EQ, 1);
  if (Match(r, ">"))  return MakeToken(r, TOKEN_GT, 1);
  if (Match(r, "<"))  return MakeToken(r, TOKEN_LT, 1);
  if (Match(r, ":"))  return MakeToken(r, TOKEN_COLON, 1);
  if (Match(r, "\n")) return MakeToken(r, TOKEN_NEWLINE, 1);
  if (Match(r, "\r")) return MakeToken(r, TOKEN_NEWLINE, 1);

  if (IsSymChar(Peek(r))) return IdentifierToken(r);

  return ErrorToken(r, "Unexpected character");
}

const char *TokenStr(TokenType type)
{
  switch (type) {
  case TOKEN_COLON:       return "COLON";
  case TOKEN_LPAREN:      return "LPAREN";
  case TOKEN_RPAREN:      return "RPAREN";
  case TOKEN_LBRACKET:    return "LBRACKET";
  case TOKEN_RBRACKET:    return "RBRACKET";
  case TOKEN_LBRACE:      return "LBRACE";
  case TOKEN_RBRACE:      return "RBRACE";
  case TOKEN_COMMA:       return "COMMA";
  case TOKEN_DOT:         return "DOT";
  case TOKEN_MINUS:       return "MINUS";
  case TOKEN_PLUS:        return "PLUS";
  case TOKEN_STAR:        return "STAR";
  case TOKEN_SLASH:       return "SLASH";
  case TOKEN_EXPONENT:    return "EXPONENT";
  case TOKEN_BAR:         return "BAR";
  case TOKEN_EQ:          return "EQ";
  case TOKEN_NEQ:         return "NEQ";
  case TOKEN_GT:          return "GT";
  case TOKEN_GTE:         return "GTE";
  case TOKEN_LT:          return "LT";
  case TOKEN_LTE:         return "LTE";
  case TOKEN_PIPE:        return "PIPE";
  case TOKEN_ARROW:       return "ARROW";
  case TOKEN_IDENTIFIER:  return "IDENTIFIER";
  case TOKEN_STRING:      return "STRING";
  case TOKEN_NUMBER:      return "NUMBER";
  case TOKEN_SYMBOL:      return "SYMBOL";
  case TOKEN_AND:         return "AND";
  case TOKEN_NOT:         return "NOT";
  case TOKEN_OR:          return "OR";
  case TOKEN_DEF:         return "DEF";
  case TOKEN_COND:        return "COND";
  case TOKEN_DO:          return "DO";
  case TOKEN_ELSE:        return "ELSE";
  case TOKEN_END:         return "END";
  case TOKEN_TRUE:        return "TRUE";
  case TOKEN_FALSE:       return "FALSE";
  case TOKEN_NIL:         return "NIL";
  case TOKEN_NEWLINE:     return "NEWLINE";
  case TOKEN_EOF:         return "EOF";
  case TOKEN_ERROR:       return "ERROR";
  }
}

void PrintSourceContext(Reader *r, u32 num_lines)
{
  char *c = &r->src[r->cur];
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    if (IsNewline(*c)) after++;
    c++;
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
