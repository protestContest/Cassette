#include "scan.h"
#include "mem.h"
#include "printer.h"
#include <stdio.h>

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define IsBoundary(c)     (IsSpace(c) || IsNewline(c) || IsEnd(c))

#define LookAhead(r, i)   ((r)->source.data[(r)->source.cur + (i)])
#define Peek(r)           LookAhead(r, 0)
#define PeekNext(r)       LookAhead(r, 1)

TokenType CurToken(Parser *r)
{
  return r->token.type;
}

static char Advance(Parser *r)
{
  r->source.col++;
  return r->source.data[r->source.cur++];
}

static void AdvanceLine(Parser *r)
{
  r->source.line++;
  r->source.col = 1;
  r->source.cur++;
}

static void SkipSpace(Parser *r)
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

static bool Check(Parser *r, const char *expect)
{
  u32 len = strlen(expect);

  for (u32 i = 0; i < len; i++) {
    if IsEnd(LookAhead(r, i)) return false;
    if (LookAhead(r, i) != expect[i]) return false;
  }

  return true;
}

static bool Match(Parser *r, const char *expect)
{
  if (Check(r, expect)) {
    for (u32 i = 0; i < strlen(expect); i++) {
      if (IsNewline(Peek(r))) AdvanceLine(r);
      else Advance(r);
    }
    return true;
  } else {
    return false;
  }
}

bool CheckKeyword(Parser *r, const char *expect)
{
  if (!Check(r, expect)) return false;
  if (IsSymChar(LookAhead(r, strlen(expect)))) return false;
  return true;
}

bool MatchKeyword(Parser *r, const char *expect)
{
  if (CheckKeyword(r, expect)) {
    Match(r, expect);
    return true;
  } else {
    return false;
  }
}

static Token MakeToken(Parser *r, TokenType type, u32 length)
{
  Token token;
  token.type = type;
  token.line = r->source.line;
  token.col = r->source.col;
  token.lexeme = r->source.data + r->source.cur - length;
  token.length = length;
  return token;
}

static Token ErrorToken(Parser *r, char *msg, ...)
{
  Token token;
  va_list vlist, vcount;
  va_start(vlist, msg);

  va_copy(vcount, vlist);
  u32 count = vsnprintf(NULL, 0, msg, vcount);
  va_end(vcount);
  token.lexeme = malloc(count + 1);

  vsprintf(token.lexeme, msg, vlist);
  va_end(vlist);

  token.type = TOKEN_ERROR;
  token.line = r->source.line;
  token.col = r->source.col;
  token.length = strlen(msg);

  return token;
}

static Token NumberToken(Parser *r)
{
  u32 start = r->source.cur;

  while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  if (Check(r, ".") && IsDigit(PeekNext(r))) {
    Match(r, ".");
    while (IsDigit(Peek(r)) || Peek(r) == '_') Advance(r);
  }

  return MakeToken(r, TOKEN_NUMBER, r->source.cur - start);
}

static Token StringToken(Parser *r)
{
  u32 start = r->source.cur - 1;
  while (Peek(r) != '"' && !IsEnd(Peek(r))) {
    if (Peek(r) == '\n') AdvanceLine(r);
    else Advance(r);
  }

  if (IsEnd(Peek(r))) return ErrorToken(r, "Unterminated string");
  Advance(r);
  return MakeToken(r, TOKEN_STRING, r->source.cur - start);
}

Token IdentifierToken(Parser *r)
{
  u32 start = r->source.cur;

  if (MatchKeyword(r, "true"))  return MakeToken(r, TOKEN_TRUE, 4);
  if (MatchKeyword(r, "false")) return MakeToken(r, TOKEN_FALSE, 5);
  if (MatchKeyword(r, "nil"))   return MakeToken(r, TOKEN_NIL, 3);
  if (MatchKeyword(r, "and"))   return MakeToken(r, TOKEN_AND, 3);
  if (MatchKeyword(r, "not"))   return MakeToken(r, TOKEN_NOT, 3);
  if (MatchKeyword(r, "cond"))  return MakeToken(r, TOKEN_COND, 4);
  if (MatchKeyword(r, "def"))   return MakeToken(r, TOKEN_DEF, 3);
  if (MatchKeyword(r, "if"))    return MakeToken(r, TOKEN_IF, 2);
  if (MatchKeyword(r, "do"))    return MakeToken(r, TOKEN_DO, 2);
  if (MatchKeyword(r, "else"))  return MakeToken(r, TOKEN_ELSE, 4);
  if (MatchKeyword(r, "end"))   return MakeToken(r, TOKEN_END, 3);
  if (MatchKeyword(r, "or"))    return MakeToken(r, TOKEN_OR, 2);

  while (IsSymChar(Peek(r))) Advance(r);

  if (r->source.cur == start) return ErrorToken(r, "Expected symbol");

  return MakeToken(r, TOKEN_IDENTIFIER, r->source.cur - start);
}

void InitParser(Parser *r, char *src, Chunk *chunk)
{
  r->status = Ok;
  r->source.line = 1;
  r->source.col = 1;
  r->source.data = src;
  r->source.cur = 0;
  r->chunk = chunk;
  r->error = NULL;
}

static Token ScanToken(Parser *r)
{
  SkipSpace(r);

  if (IsEnd(Peek(r))) return MakeToken(r, TOKEN_EOF, 0);

  if (IsDigit(Peek(r))) return NumberToken(r);
  if (Match(r, "\""))   return StringToken(r);

  if (Match(r, "!=")) return MakeToken(r, TOKEN_NEQ, 2);
  if (Match(r, ">=")) return MakeToken(r, TOKEN_GTE, 2);
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

void AdvanceToken(Parser *r)
{
  r->token = ScanToken(r);
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
  case TOKEN_AND:         return "AND";
  case TOKEN_NOT:         return "NOT";
  case TOKEN_OR:          return "OR";
  case TOKEN_DEF:         return "DEF";
  case TOKEN_COND:        return "COND";
  case TOKEN_IF:          return "IF";
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

bool MatchToken(Parser *r, TokenType type)
{
  if (r->token.type == type) {
    AdvanceToken(r);
    return true;
  }
  return false;
}

void ExpectToken(Parser *r, TokenType type)
{
  if (r->token.type == type) {
    AdvanceToken(r);
  } else {
    r->token = ErrorToken(r, "Expected %s", TokenStr(type));
    r->status = Error;
  }
}

bool TokenEnd(Parser *r, TokenType type)
{
  return r->token.type == type || r->token.type == TOKEN_EOF;
}

void PrintSourceContext(Parser *r, u32 num_lines)
{
  char *c = &Peek(r);
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    if (IsNewline(*c)) after++;
    c++;
  }

  u32 before = num_lines - after;
  if (before > r->source.line) {
    before = r->source.line;
    after = num_lines - r->source.line;
  }

  u32 gutter = (r->source.line >= 1000) ? 4 : (r->source.line >= 100) ? 3 : (r->source.line >= 10) ? 2 : 1;
  if (gutter < 2) gutter = 2;

  c = r->source.data;
  i32 line = 1;

  while (line < (i32)r->source.line - (i32)before) {
    if (IsEnd(*c)) return;
    if (IsNewline(*c)) line++;
    c++;
  }

  while (line < (i32)r->source.line + (i32)after + 1 && !IsEnd(*c)) {
    fprintf(stderr, "  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      fprintf(stderr, "%c", *c);
      c++;
    }

    if (line == (i32)r->source.line) {
      fprintf(stderr, "\n  ");
      for (u32 i = 0; i < gutter + r->source.col + 2; i++) fprintf(stderr, " ");
      fprintf(stderr, "↑");
    }
    fprintf(stderr, "\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > r->source.data && !IsNewline(*(c-1))) {
    fprintf(stderr, "\n");
  }
}

void PrintParser(Parser *p)
{
  printf("%s %.*s\n", TokenStr(p->token.type), p->token.length, p->token.lexeme);
  PrintSourceContext(p, 0);
}
