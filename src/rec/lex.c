#include "rec.h"
#include "mem/mem.h"

#define Peek(lex)     (lex)->source[(lex)->pos]
#define IsSpace(c)    ((c) == ' ' || (c) == '\t')
#define IsDigit(c)    ((c) >= '0' && (c) <= '9')
#define IsHexDigit(c) (IsDigit(c) || ((c) >= 'A' && (c) <= 'F'))

static bool IsSymChar(char c);
static void SkipWhitespace(Lexer *lex);
static bool Match(char *test, Lexer *lex);
static bool MatchKeyword(char *test, Lexer *lex);
static Token MakeToken(TokenType type, char *lexeme, u32 length);
static Token NumberToken(Lexer *lex);
static Token StringToken(Lexer *lex);
static Token KeywordToken(Lexer *lex);

void InitLexer(Lexer *lex, char *source)
{
  lex->source = source;
  lex->pos = 0;
  lex->token = NextToken(lex);
}

Token NextToken(Lexer *lex)
{
  char *start;

  SkipWhitespace(lex);
  start = lex->source + lex->pos;

  if (Peek(lex) == '\0') return MakeToken(TokenEOF, start, 0);
  if (IsDigit(Peek(lex))) return NumberToken(lex);
  if (Peek(lex) == '"') return StringToken(lex);

  if (Match("!=", lex)) return MakeToken(TokenBangEqual, start, 2);
  if (Match("->", lex)) return MakeToken(TokenArrow, start, 2);
  if (Match("<=", lex)) return MakeToken(TokenLessEqual, start, 2);
  if (Match("==", lex)) return MakeToken(TokenEqualEqual, start, 2);
  if (Match(">=", lex)) return MakeToken(TokenGreaterEqual, start, 2);
  if (Match("\n", lex)) return MakeToken(TokenNewline, start, 1);
  if (Match("#", lex))  return MakeToken(TokenHash, start, 1);
  if (Match("%", lex))  return MakeToken(TokenPercent, start, 1);
  if (Match("(", lex))  return MakeToken(TokenLParen, start, 1);
  if (Match(")", lex))  return MakeToken(TokenRParen, start, 1);
  if (Match("*", lex))  return MakeToken(TokenStar, start, 1);
  if (Match("+", lex))  return MakeToken(TokenPlus, start, 1);
  if (Match(",", lex))  return MakeToken(TokenComma, start, 1);
  if (Match("-", lex))  return MakeToken(TokenMinus, start, 1);
  if (Match(".", lex))  return MakeToken(TokenDot, start, 1);
  if (Match("/", lex))  return MakeToken(TokenSlash, start, 1);
  if (Match(":", lex))  return MakeToken(TokenColon, start, 1);
  if (Match("<", lex))  return MakeToken(TokenLess, start, 1);
  if (Match("=", lex))  return MakeToken(TokenEqual, start, 1);
  if (Match(">", lex))  return MakeToken(TokenGreater, start, 1);
  if (Match("[", lex))  return MakeToken(TokenLBracket, start, 1);
  if (Match("]", lex))  return MakeToken(TokenRBracket, start, 1);
  if (Match("{", lex))  return MakeToken(TokenLBrace, start, 1);
  if (Match("|", lex))  return MakeToken(TokenBar, start, 1);
  if (Match("}", lex))  return MakeToken(TokenRBrace, start, 1);

  return KeywordToken(lex);
}

static bool IsSymChar(char c)
{
  switch (c) {
  case ';':
  case '#':
  case '%':
  case '(':
  case ')':
  case '*':
  case '+':
  case ',':
  case '-':
  case '.':
  case '/':
  case ':':
  case '<':
  case '=':
  case '>':
  case '[':
  case ']':
  case '{':
  case '|':
  case '}':
    return false;
  default:
    return true;
  }
}

static void SkipWhitespace(Lexer *lex)
{
  while (IsSpace(Peek(lex))) lex->pos++;
  if (Peek(lex) == ';') {
    while (Peek(lex) != '\n' && Peek(lex) != '\0') lex->pos++;
    if (Peek(lex) == '\n') lex->pos++;
    SkipWhitespace(lex);
  }
}

static bool Match(char *test, Lexer *lex)
{
  u32 start = lex->pos;
  while (*test != '\0' && lex->source[lex->pos] != '\0') {
    if (*test != lex->source[lex->pos]) {
      lex->pos = start;
      return false;
    }

    test++;
    lex->pos++;
  }
  return *test == '\0';
}

static bool MatchKeyword(char *test, Lexer *lex)
{
  u32 start = lex->pos;
  if (Match(test, lex) && !IsSymChar(Peek(lex))) {
    return true;
  }
  lex->pos = start;
  return false;
}

static Token MakeToken(TokenType type, char *lexeme, u32 length)
{
  Token token;
  token.type = type;
  token.lexeme = lexeme;
  token.length = length;
  return token;
}

static Token NumberToken(Lexer *lex)
{
  u32 start = lex->pos;
  if (Match("0x", lex) && IsDigit(Peek(lex))) {
    while (IsHexDigit(Peek(lex))) lex->pos++;
    return MakeToken(TokenNum, lex->source + start, lex->pos - start);
  } else {
    lex->pos = start;
  }

  while (IsDigit(Peek(lex))) lex->pos++;
  if (Peek(lex) == '.' && IsDigit(lex->source[lex->pos+1])) {
    lex->pos++;
    while (IsDigit(Peek(lex))) lex->pos++;
  }

  return MakeToken(TokenNum, lex->source + start, lex->pos - start);
}

static Token StringToken(Lexer *lex)
{
  u32 start = lex->pos;
  lex->pos++;
  while (Peek(lex) != '"') {
    if (Peek(lex) == '\'') lex->pos++;
    if (Peek(lex) == '\0') break;
    lex->pos++;
  }
  Match("\"", lex);
  return MakeToken(TokenString, lex->source + start, lex->pos - start);
}

static Token KeywordToken(Lexer *lex)
{
  u32 start = lex->pos;

  if (MatchKeyword("and", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("as", lex))      return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("cond", lex))    return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("def", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("do", lex))      return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("else", lex))    return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("end", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("false", lex))   return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("if", lex))      return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("import", lex))  return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("in", lex))      return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("let", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("module", lex))  return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("nil", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("not", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("or", lex))      return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
  if (MatchKeyword("true", lex))    return MakeToken(TokenAnd, lex->source + start, lex->pos - start);

  while (IsSymChar(Peek(lex))) lex->pos++;
  return MakeToken(TokenID, lex->source + start, lex->pos - start);
}
