#include "lex.h"
#include <univ/io.h>
#include <stdio.h>
#include <assert.h>

bool IsIDChar(char c)
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

void SkipWhitespace(Lexer *lexer)
{
  while (IsWhitespace(LexPeek(lexer, 0))) {
    if (IsNewline(LexPeek(lexer, 0))) {
      lexer->line++;
      lexer->col = 1;
    } else {
      lexer->col++;
    }
    lexer->pos++;
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

  if (!IsWhitespace(LexPeek(lexer, i + 1))) return false;

  lexer->pos += i;
  return true;
}

Token MakeToken(u32 type, Lexer *lexer)
{
  Token token = (Token){type, lexer->line, lexer->col, lexer->src + lexer->start, lexer->pos - lexer->start};
  return token;
}

Token ErrorToken(Lexer *lexer, char *msg)
{
  char *c = msg;
  while (*c != '\0') c++;
  Token token = {-1, lexer->line, lexer->col, msg, c - msg};
  return token;
}

static Token NumberToken(Lexer *lexer)
{
  if (Match(lexer, "0x") && IsHexDigit(LexPeek(lexer, 2))) {
    while (IsHexDigit(LexPeek(lexer, 0))) lexer->pos++;
    return MakeToken(TOKEN_NUMBER, lexer);
  } else if (Match(lexer, "0b") && (LexPeek(lexer, 2) == '0' || LexPeek(lexer, 2) == '1')) {
    while (LexPeek(lexer, 0) == '0' || LexPeek(lexer, 0) == '1') lexer->pos++;
    return MakeToken(TOKEN_NUMBER, lexer);
  }

  while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;

  if (LexPeek(lexer, 0) == '.' && IsDigit(LexPeek(lexer, 1))) {
    lexer->pos++;
    while (IsDigit(LexPeek(lexer, 0))) lexer->pos++;
  }

  return MakeToken(TOKEN_NUMBER, lexer);
}

// static Token StringToken(Lexer *lexer)
// {
//   Match(lexer, "\"");
//   while (LexPeek(lexer, 0) != '"') {
//     if (LexPeek(lexer, 0) == '\\') lexer->pos++;
//     if (LexPeek(lexer, 0) == '\0') break;
//     lexer->pos++;
//   }
//   Match(lexer, "\"");
//   return MakeToken(TOKEN_STRING, lexer);
// }

static Token IDToken(Lexer *lexer)
{
  while (IsIDChar(LexPeek(lexer, 0))) lexer->pos++;
  return MakeToken(TOKEN_ID, lexer);
}

void InitLexer(Lexer *lexer, NextTokenFn next_token, char *src)
{
  *lexer = (Lexer){src, 0, 0, 1, 1, next_token};
}

Token NextToken(Lexer *lexer)
{
  SkipWhitespace(lexer);
  lexer->start = lexer->pos;
  return lexer->next_token(lexer);
}

Token RyeToken(Lexer *lexer)
{
  char c = LexPeek(lexer, 0);

  if (c == '\0') return MakeToken(TOKEN_EOF, lexer);
  if (IsDigit(c)) return NumberToken(lexer);

  // if (Match(lexer, "->"))  return MakeToken(TOKEN_ARROW, lexer);
  if (Match(lexer, "("))  return MakeToken(TOKEN_LPAREN, lexer);
  if (Match(lexer, ")"))  return MakeToken(TOKEN_RPAREN, lexer);
  // if (Match(lexer, "["))  return MakeToken(TOKEN_LBRACKET, lexer);
  // if (Match(lexer, "]"))  return MakeToken(TOKEN_RBRACKET, lexer);
  // if (Match(lexer, "{"))  return MakeToken(TOKEN_LBRACE, lexer);
  // if (Match(lexer, "}"))  return MakeToken(TOKEN_RBRACE, lexer);
  // if (Match(lexer, "."))  return MakeToken(TOKEN_DOT, lexer);
  if (Match(lexer, "*"))  return MakeToken(TOKEN_STAR, lexer);
  if (Match(lexer, "/"))  return MakeToken(TOKEN_SLASH, lexer);
  if (Match(lexer, "-"))  return MakeToken(TOKEN_MINUS, lexer);
  if (Match(lexer, "+"))  return MakeToken(TOKEN_PLUS, lexer);
  // if (Match(lexer, "|"))  return MakeToken(TOKEN_BAR, lexer);
  // if (Match(lexer, ">"))  return MakeToken(TOKEN_GREATER, lexer);
  // if (Match(lexer, "<"))  return MakeToken(TOKEN_LESS, lexer);
  // if (Match(lexer, "="))  return MakeToken(TOKEN_EQUAL, lexer);
  // if (Match(lexer, ","))  return MakeToken(TOKEN_COMMA, lexer);
  // if (Match(lexer, "::"))  return MakeToken(TOKEN_COLON_COLON, lexer);
  // if (Match(lexer, ":"))  return MakeToken(TOKEN_COLON, lexer);
  // if (Match(lexer, "\n"))  return MakeToken(TOKEN_NEWLINE, lexer);

  // if (MatchKeyword(lexer, "and"))   return MakeToken(TOKEN_AND, lexer);
  // if (MatchKeyword(lexer, "cond"))  return MakeToken(TOKEN_COND, lexer);
  // if (MatchKeyword(lexer, "def"))   return MakeToken(TOKEN_DEF, lexer);
  // if (MatchKeyword(lexer, "do"))    return MakeToken(TOKEN_DO, lexer);
  // if (MatchKeyword(lexer, "else"))  return MakeToken(TOKEN_ELSE, lexer);
  // if (MatchKeyword(lexer, "end"))   return MakeToken(TOKEN_END, lexer);
  // if (MatchKeyword(lexer, "if"))    return MakeToken(TOKEN_IF, lexer);
  // if (MatchKeyword(lexer, "not"))   return MakeToken(TOKEN_NOT, lexer);

  return IDToken(lexer);
}

int PrintToken(Token token)
{
  return printf("%.*s", token.length, token.lexeme);
}

int DebugToken(Token token)
{
  if (token.length > 0) {
    return printf("[%d %.*s]", token.type, token.length, token.lexeme);
  } else {
    return printf("[%d]", token.type);
  }
}

void PrintSourceContext(Lexer *lexer, u32 num_lines)
{
  u32 before = num_lines;
  if (before > lexer->line - 1) before = lexer->line - 1;
  u32 after = num_lines - before;

  u32 start = lexer->line - before;
  u32 end = lexer->line + after;

  // printf("%d %d\n", before, after);

  char *c = lexer->src;
  u32 line = 1;
  while (*c != '\0' && line < start) {
    if (IsNewline(*c)) line++;
    c++;
  }

  while (*c != '\0' && line <= end) {
    if (line == lexer->line) {
      printf("→%3d  ", line);
    } else {
      printf("%4d  ", line);
    }

    while (*c != '\0' && !IsNewline(*c)) {
      if (c - lexer->src == lexer->start && lexer->start < lexer->pos) {
        printf("%s", IOUnderline);
      }
      printf("%c", *c);
      c++;
      if (c - lexer->src == lexer->pos && lexer->start < lexer->pos) {
        printf("%s", IONoUnderline);
      }
    }
    printf("\n");
    c++;

    line++;
  }
}
