#include "scan.h"
#include "compile.h"
#include "../platform/console.h"
#include "../platform/string.h"
#include "../platform/allocate.h"

#define IsSpace(c)        ((c) == ' ' || (c) == '\t')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' & (c) <= 'z'))
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsEnd(c)          ((c) == '\0')
#define IsBoundary(c)     (IsSpace(c) || IsNewline(c) || IsEnd(c))

#define LookAhead(p, i)   ((p)->source.data[(p)->source.cur + (i)])
#define Peek(p)           LookAhead(p, 0)
#define PeekNext(p)       LookAhead(p, 1)

TokenType CurToken(Parser *p)
{
  return p->token.type;
}

static char Advance(Parser *p)
{
  p->source.col++;
  return p->source.data[p->source.cur++];
}

static void AdvanceLine(Parser *p)
{
  p->source.line++;
  p->source.col = 1;
  p->source.cur++;
}

static void SkipSpace(Parser *p)
{
  while (IsSpace(Peek(p))) {
    Advance(p);
  }

  if (Peek(p) == ';') {
    while (!IsNewline(Peek(p))) {
      Advance(p);
    }
    AdvanceLine(p);
    SkipSpace(p);
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

static bool Check(Parser *p, char *expect)
{
  u32 len = Strlen(expect);

  for (u32 i = 0; i < len; i++) {
    if IsEnd(LookAhead(p, i)) return false;
    if (LookAhead(p, i) != expect[i]) return false;
  }

  return true;
}

static bool Match(Parser *p, char *expect)
{
  if (Check(p, expect)) {
    for (u32 i = 0; i < Strlen(expect); i++) {
      if (IsNewline(Peek(p))) AdvanceLine(p);
      else Advance(p);
    }
    return true;
  } else {
    return false;
  }
}

bool CheckKeyword(Parser *p, char *expect)
{
  if (!Check(p, expect)) return false;
  if (IsSymChar(LookAhead(p, Strlen(expect)))) return false;
  return true;
}

bool MatchKeyword(Parser *p, char *expect)
{
  if (CheckKeyword(p, expect)) {
    Match(p, expect);
    return true;
  } else {
    return false;
  }
}

static Token MakeToken(Parser *p, TokenType type, u32 length)
{
  Token token;
  token.type = type;
  token.line = p->source.line;
  token.col = p->source.col;
  token.lexeme = p->source.data + p->source.cur - length;
  token.length = length;
  return token;
}

static Token NumberToken(Parser *p)
{
  u32 start = p->source.cur;

  while (IsDigit(Peek(p)) || Peek(p) == '_') Advance(p);
  if (Check(p, ".") && IsDigit(PeekNext(p))) {
    Match(p, ".");
    while (IsDigit(Peek(p)) || Peek(p) == '_') Advance(p);
  }

  return MakeToken(p, TOKEN_NUMBER, p->source.cur - start);
}

static Token StringToken(Parser *p)
{
  u32 start = p->source.cur - 1;
  while (Peek(p) != '"' && !IsEnd(Peek(p))) {
    if (Peek(p) == '\n') AdvanceLine(p);
    else Advance(p);
  }

  if (IsEnd(Peek(p))) {
    p->status = Error;
  }

  Advance(p);
  return MakeToken(p, TOKEN_STRING, p->source.cur - start);
}

Token IdentifierToken(Parser *p)
{
  u32 start = p->source.cur;

  if (MatchKeyword(p, "module"))  return MakeToken(p, TOKEN_MODULE, 6);
  if (MatchKeyword(p, "import"))  return MakeToken(p, TOKEN_IMPORT, 6);
  if (MatchKeyword(p, "true"))    return MakeToken(p, TOKEN_TRUE, 4);
  if (MatchKeyword(p, "false"))   return MakeToken(p, TOKEN_FALSE, 5);
  if (MatchKeyword(p, "nil"))     return MakeToken(p, TOKEN_NIL, 3);
  if (MatchKeyword(p, "and"))     return MakeToken(p, TOKEN_AND, 3);
  if (MatchKeyword(p, "not"))     return MakeToken(p, TOKEN_NOT, 3);
  if (MatchKeyword(p, "cond"))    return MakeToken(p, TOKEN_COND, 4);
  if (MatchKeyword(p, "def"))     return MakeToken(p, TOKEN_DEF, 3);
  if (MatchKeyword(p, "if"))      return MakeToken(p, TOKEN_IF, 2);
  if (MatchKeyword(p, "do"))      return MakeToken(p, TOKEN_DO, 2);
  if (MatchKeyword(p, "else"))    return MakeToken(p, TOKEN_ELSE, 4);
  if (MatchKeyword(p, "end"))     return MakeToken(p, TOKEN_END, 3);
  if (MatchKeyword(p, "or"))      return MakeToken(p, TOKEN_OR, 2);
  if (MatchKeyword(p, "let"))     return MakeToken(p, TOKEN_LET, 3);
  if (MatchKeyword(p, "use"))     return MakeToken(p, TOKEN_USE, 3);

  while (IsSymChar(Peek(p))) Advance(p);

  if (p->source.cur == start) {
    p->status = Error;
  }

  return MakeToken(p, TOKEN_IDENTIFIER, p->source.cur - start);
}

static Token ScanToken(Parser *p)
{
  SkipSpace(p);

  if (IsEnd(Peek(p))) return MakeToken(p, TOKEN_EOF, 0);

  if (IsDigit(Peek(p))) return NumberToken(p);
  if (Match(p, "\""))   return StringToken(p);

  if (Match(p, "!=")) return MakeToken(p, TOKEN_NEQ, 2);
  if (Match(p, ">=")) return MakeToken(p, TOKEN_GTE, 2);
  if (Match(p, "|>")) return MakeToken(p, TOKEN_PIPE, 2);
  if (Match(p, "->")) return MakeToken(p, TOKEN_ARROW, 2);
  if (Match(p, "<=")) return MakeToken(p, TOKEN_LTE, 2);

  if (Match(p, "("))  return MakeToken(p, TOKEN_LPAREN, 1);
  if (Match(p, ")"))  return MakeToken(p, TOKEN_RPAREN, 1);
  if (Match(p, "["))  return MakeToken(p, TOKEN_LBRACKET, 1);
  if (Match(p, "]"))  return MakeToken(p, TOKEN_RBRACKET, 1);
  if (Match(p, "{"))  return MakeToken(p, TOKEN_LBRACE, 1);
  if (Match(p, "}"))  return MakeToken(p, TOKEN_RBRACE, 1);
  if (Match(p, ","))  return MakeToken(p, TOKEN_COMMA, 1);
  if (Match(p, "."))  return MakeToken(p, TOKEN_DOT, 1);
  if (Match(p, "-"))  return MakeToken(p, TOKEN_MINUS, 1);
  if (Match(p, "+"))  return MakeToken(p, TOKEN_PLUS, 1);
  if (Match(p, "*"))  return MakeToken(p, TOKEN_STAR, 1);
  if (Match(p, "/"))  return MakeToken(p, TOKEN_SLASH, 1);
  if (Match(p, "|"))  return MakeToken(p, TOKEN_BAR, 1);
  if (Match(p, "="))  return MakeToken(p, TOKEN_EQ, 1);
  if (Match(p, ">"))  return MakeToken(p, TOKEN_GT, 1);
  if (Match(p, "<"))  return MakeToken(p, TOKEN_LT, 1);
  if (Match(p, ":"))  return MakeToken(p, TOKEN_COLON, 1);
  if (Match(p, "\n")) return MakeToken(p, TOKEN_NEWLINE, 1);
  if (Match(p, "\r")) return MakeToken(p, TOKEN_NEWLINE, 1);

  if (IsSymChar(Peek(p))) return IdentifierToken(p);

  p->status = Error;
}

void AdvanceToken(Parser *p)
{
  p->token = ScanToken(p);
}

char *TokenStr(TokenType type)
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
  case TOKEN_MODULE:      return "MODULE";
  case TOKEN_IMPORT:      return "IMPORT";
  case TOKEN_LET:         return "LET";
  case TOKEN_USE:         return "USE";
  }
}

bool MatchToken(Parser *p, TokenType type)
{
  if (p->token.type == type) {
    AdvanceToken(p);
    return true;
  }
  return false;
}

void ExpectToken(Parser *p, TokenType type)
{
  if (p->token.type == type) {
    AdvanceToken(p);
  } else {
    p->status = Error;
  }
}

bool TokenEnd(Parser *p, TokenType type)
{
  return p->token.type == type || p->token.type == TOKEN_EOF;
}

void PrintSourceContext(Parser *p, u32 num_lines)
{
  char *c = &Peek(p);
  u32 after = 0;
  while (!IsEnd(*c) && after < num_lines / 2) {
    if (IsNewline(*c)) after++;
    c++;
  }

  u32 before = num_lines - after;
  if (before > p->source.line) {
    before = p->source.line;
    after = num_lines - p->source.line;
  }

  u32 gutter = (p->source.line >= 1000) ? 4 : (p->source.line >= 100) ? 3 : (p->source.line >= 10) ? 2 : 1;
  if (gutter < 2) gutter = 2;

  c = p->source.data;
  i32 line = 1;

  while (line < (i32)p->source.line - (i32)before) {
    if (IsEnd(*c)) return;
    if (IsNewline(*c)) line++;
    c++;
  }

  Print("  %*d │ ", gutter, line);
  while (!IsNewline(*c) && !IsEnd(*c)) {
    if (c == p->token.lexeme) {
      Print("\x1B[4m");
    }
    if (c == p->token.lexeme + p->token.length) {
      Print("\x1B[0m");
    }
    Print("%c", *c++);
  }
  Print("\x1B[0m");
  Print("\n");
  Print("%*s↑\n", p->source.col + gutter + 4, "");
  line++;
  c++;

  while (line < (i32)p->source.line + (i32)after + 1 && !IsEnd(*c)) {
    Print("  %*d │ ", gutter, line);
    while (!IsNewline(*c) && !IsEnd(*c)) {
      Print("%c", *c);
      c++;
    }

    Print("\n");
    line++;
    c++;
  }

  if (IsEnd(*c) && c > p->source.data && !IsNewline(*(c-1))) {
    Print("\n");
  }
}

void PrintParser(Parser *p)
{
  Print("[%d:%d] %s %.*s\n", p->source.line, p->source.col, TokenStr(p->token.type), p->token.length, p->token.lexeme);
  PrintSourceContext(p, 0);
}

void SkipNewlines(Parser *p)
{
  while (CurToken(p) == TOKEN_NEWLINE) AdvanceToken(p);
}
