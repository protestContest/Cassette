#include "lex.h"
#include <stdio.h>
#include <assert.h>

#define Peek(lexer, n)    (lexer)->src[(lexer)->pos + n]
#define IsWhitespace(c)   ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')
#define IsNewline(c)      ((c) == '\r' || (c) == '\n')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c)     (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))

static bool IsIDChar(char c)
{
  if (IsWhitespace(c)) return false;

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
  while (IsWhitespace(Peek(lexer, 0))) {
    if (IsNewline(Peek(lexer, 0))) {
      lexer->line++;
      lexer->col = 1;
    } else {
      lexer->col++;
    }
    lexer->pos++;
  }
}

static bool Match(Lexer *lexer, char *expected)
{
  u32 i = 0;
  char *c = expected;
  while (*c != '\0') {
    if (Peek(lexer, i) != *c) return false;
    i++;
    c++;
  }
  lexer->pos += i;
  return true;
}

static bool MatchKeyword(Lexer *lexer, char *expected)
{
  u32 i = 0;
  char *c = expected;
  while (*c != '\0') {
    if (Peek(lexer, i) != *c) return false;
    i++;
    c++;
  }

  if (!IsWhitespace(Peek(lexer, i + 1))) return false;

  lexer->pos += i;
  return true;
}

static Token MakeToken(TokenType type, u32 start, Lexer *lexer)
{
  Token token = (Token){type, lexer->line, lexer->col, lexer->src + start, lexer->pos - start};
  return token;
}

static Token NumberToken(Lexer *lexer)
{
  u32 start = lexer->pos;

  if (Match(lexer, "0x") && IsHexDigit(Peek(lexer, 2))) {
    while (IsHexDigit(Peek(lexer, 0))) lexer->pos++;
    return MakeToken(TOKEN_NUMBER, start, lexer);
  } else if (Match(lexer, "0b") && (Peek(lexer, 2) == '0' || Peek(lexer, 2) == '1')) {
    while (Peek(lexer, 0) == '0' || Peek(lexer, 0) == '1') lexer->pos++;
    return MakeToken(TOKEN_NUMBER, start, lexer);
  }

  while (IsDigit(Peek(lexer, 0))) lexer->pos++;

  if (Peek(lexer, 0) == '.' && IsDigit(Peek(lexer, 1))) {
    lexer->pos++;
    while (IsDigit(Peek(lexer, 0))) lexer->pos++;
  }

  return MakeToken(TOKEN_NUMBER, start, lexer);
}

static Token StringToken(Lexer *lexer)
{
  u32 start = lexer->pos;
  Match(lexer, "\"");
  while (Peek(lexer, 0) != '"') {
    if (Peek(lexer, 0) == '\\') lexer->pos++;
    if (Peek(lexer, 0) == '\0') break;
    lexer->pos++;
  }
  Match(lexer, "\"");
  return MakeToken(TOKEN_STRING, start, lexer);
}

static Token IDToken(Lexer *lexer)
{
  u32 start = lexer->pos;
  while (IsIDChar(Peek(lexer, 0))) lexer->pos++;
  return MakeToken(TOKEN_ID, start, lexer);
}

void InitLexer(Lexer *lexer, char *src)
{
  *lexer = (Lexer){src, 0, 1, 1};
}

Token NextToken(Lexer *lexer)
{
  SkipWhitespace(lexer);

  char c = Peek(lexer, 0);

  if (c == '\0') return MakeToken(TOKEN_EOF, lexer->pos, lexer);
  if (IsDigit(c)) return NumberToken(lexer);

  u32 start = lexer->pos;
  if (Match(lexer, "->"))  return MakeToken(TOKEN_ARROW, start, lexer);
  if (Match(lexer, "("))  return MakeToken(TOKEN_LPAREN, start, lexer);
  if (Match(lexer, ")"))  return MakeToken(TOKEN_RPAREN, start, lexer);
  if (Match(lexer, "["))  return MakeToken(TOKEN_LBRACKET, start, lexer);
  if (Match(lexer, "]"))  return MakeToken(TOKEN_RBRACKET, start, lexer);
  if (Match(lexer, "{"))  return MakeToken(TOKEN_LBRACE, start, lexer);
  if (Match(lexer, "}"))  return MakeToken(TOKEN_RBRACE, start, lexer);
  if (Match(lexer, "."))  return MakeToken(TOKEN_DOT, start, lexer);
  if (Match(lexer, "*"))  return MakeToken(TOKEN_STAR, start, lexer);
  if (Match(lexer, "/"))  return MakeToken(TOKEN_SLASH, start, lexer);
  if (Match(lexer, "-"))  return MakeToken(TOKEN_MINUS, start, lexer);
  if (Match(lexer, "+"))  return MakeToken(TOKEN_PLUS, start, lexer);
  if (Match(lexer, "|"))  return MakeToken(TOKEN_BAR, start, lexer);
  if (Match(lexer, ">"))  return MakeToken(TOKEN_GREATER, start, lexer);
  if (Match(lexer, "<"))  return MakeToken(TOKEN_LESS, start, lexer);
  if (Match(lexer, "="))  return MakeToken(TOKEN_EQUAL, start, lexer);
  if (Match(lexer, ","))  return MakeToken(TOKEN_COMMA, start, lexer);
  if (Match(lexer, ":"))  return MakeToken(TOKEN_COLON, start, lexer);
  // if (Match(lexer, "\n"))  return MakeToken(TOKEN_NEWLINE, start, lexer);

  if (MatchKeyword(lexer, "and"))   return MakeToken(TOKEN_AND, start, lexer);
  if (MatchKeyword(lexer, "cond"))  return MakeToken(TOKEN_COND, start, lexer);
  if (MatchKeyword(lexer, "def"))   return MakeToken(TOKEN_DEF, start, lexer);
  if (MatchKeyword(lexer, "do"))    return MakeToken(TOKEN_DO, start, lexer);
  if (MatchKeyword(lexer, "else"))  return MakeToken(TOKEN_ELSE, start, lexer);
  if (MatchKeyword(lexer, "end"))   return MakeToken(TOKEN_END, start, lexer);
  if (MatchKeyword(lexer, "if"))    return MakeToken(TOKEN_IF, start, lexer);
  if (MatchKeyword(lexer, "not"))   return MakeToken(TOKEN_NOT, start, lexer);

  return IDToken(lexer);
}

void PrintToken(Token token)
{
  switch (token.type) {
  case TOKEN_EOF:
    printf("EOF");
    break;
  case TOKEN_ERROR:
    printf("ERROR [%.*s]", token.length, token.lexeme);
    break;
  case TOKEN_NUMBER:
    printf("NUMBER [%.*s]", token.length, token.lexeme);
    break;
  case TOKEN_STRING:
    printf("STRING [%.*s]", token.length, token.lexeme);
    break;
  case TOKEN_ID:
    printf("ID [%.*s]", token.length, token.lexeme);
    break;
  case TOKEN_LPAREN:
    printf("LPAREN");
    break;
  case TOKEN_RPAREN:
    printf("RPAREN");
    break;
  case TOKEN_LBRACKET:
    printf("LBRACKET");
    break;
  case TOKEN_RBRACKET:
    printf("RBRACKET");
    break;
  case TOKEN_LBRACE:
    printf("LBRACE");
    break;
  case TOKEN_RBRACE:
    printf("RBRACE");
    break;
  case TOKEN_DOT:
    printf("DOT");
    break;
  case TOKEN_STAR:
    printf("STAR");
    break;
  case TOKEN_SLASH:
    printf("SLASH");
    break;
  case TOKEN_MINUS:
    printf("MINUS");
    break;
  case TOKEN_PLUS:
    printf("PLUS");
    break;
  case TOKEN_BAR:
    printf("BAR");
    break;
  case TOKEN_GREATER:
    printf("GREATER");
    break;
  case TOKEN_LESS:
    printf("LESS");
    break;
  case TOKEN_EQUAL:
    printf("EQUAL");
    break;
  case TOKEN_COMMA:
    printf("COMMA");
    break;
  case TOKEN_COLON:
    printf("COLON");
    break;
  case TOKEN_NEWLINE:
    printf("NEWLINE");
    break;
  case TOKEN_ARROW:
    printf("ARROW");
    break;
  case TOKEN_AND:
    printf("AND");
    break;
  case TOKEN_COND:
    printf("COND");
    break;
  case TOKEN_DEF:
    printf("DEF");
    break;
  case TOKEN_DO:
    printf("DO");
    break;
  case TOKEN_ELSE:
    printf("ELSE");
    break;
  case TOKEN_END:
    printf("END");
    break;
  case TOKEN_IF:
    printf("IF");
    break;
  case TOKEN_NOT:
    printf("NOT");
    break;
  case TOKEN_OR:
    printf("OR");
    break;
  }
}
