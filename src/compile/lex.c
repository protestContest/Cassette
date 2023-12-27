#include "lex.h"
#include "mem/mem.h"
#include "univ/str.h"

#define Peek(lex)     (lex)->source[(lex)->pos]
#define Peek2(lex)    (lex)->source[(lex)->pos+1]

static bool IsSymChar(char c);
static void SkipWhitespace(Lexer *lex);
static bool Match(char *test, Lexer *lex);
static bool MatchKeyword(char *test, Lexer *lex);
static Token AdvanceToken(Lexer *lex);
static Token MakeToken(TokenType type, char *lexeme, u32 length);
static Token NumberToken(Lexer *lex);
static Token StringToken(Lexer *lex);
static Token KeywordToken(Lexer *lex);

void InitLexer(Lexer *lex, char *source, u32 pos)
{
  lex->source = source;
  lex->pos = pos;
  lex->token = AdvanceToken(lex);
}

Token NextToken(Lexer *lex)
{
  Token token = lex->token;
  lex->token = AdvanceToken(lex);
  return token;
}

bool MatchToken(TokenType type, Lexer *lex)
{
  if (lex->token.type == TokenEOF) {
    return type == TokenEOF;
  } else if (lex->token.type == type) {
    NextToken(lex);
    return true;
  } else {
    return false;
  }
}

void SkipNewlines(Lexer *lex)
{
  while (lex->token.type == TokenNewline) NextToken(lex);
}

static Token AdvanceToken(Lexer *lex)
{
  char *start;

  SkipWhitespace(lex);
  start = lex->source + lex->pos;

  if (Peek(lex) == '\0') return MakeToken(TokenEOF, start, 0);
  if (IsDigit(Peek(lex)) || Peek(lex) == '$') return NumberToken(lex);
  if (Peek(lex) == '"') return StringToken(lex);

  if (Match("\n", lex)) return MakeToken(TokenNewline, start, 1);
  if (Match("!=", lex)) return MakeToken(TokenBangEq, start, 2);
  if (Match("->", lex)) return MakeToken(TokenArrow, start, 2);
  if (Match("<<", lex)) return MakeToken(TokenLtLt, start, 2);
  if (Match("<=", lex)) return MakeToken(TokenLtEq, start, 2);
  if (Match("<>", lex)) return MakeToken(TokenLtGt, start, 2);
  if (Match("==", lex)) return MakeToken(TokenEqEq, start, 2);
  if (Match(">=", lex)) return MakeToken(TokenGtEq, start, 2);
  if (Match(">>", lex)) return MakeToken(TokenGtGt, start, 2);

  if (Match("#", lex))  return MakeToken(TokenHash, start, 1);
  if (Match("%", lex))  return MakeToken(TokenPercent, start, 1);
  if (Match("&", lex))  return MakeToken(TokenAmpersand, start, 1);
  if (Match("(", lex))  return MakeToken(TokenLParen, start, 1);
  if (Match(")", lex))  return MakeToken(TokenRParen, start, 1);
  if (Match("*", lex))  return MakeToken(TokenStar, start, 1);
  if (Match("+", lex))  return MakeToken(TokenPlus, start, 1);
  if (Match(",", lex))  return MakeToken(TokenComma, start, 1);
  if (Match("-", lex))  return MakeToken(TokenMinus, start, 1);
  if (Match(".", lex))  return MakeToken(TokenDot, start, 1);
  if (Match("/", lex))  return MakeToken(TokenSlash, start, 1);
  if (Match(":", lex))  return MakeToken(TokenColon, start, 1);
  if (Match("<", lex))  return MakeToken(TokenLt, start, 1);
  if (Match("=", lex))  return MakeToken(TokenEq, start, 1);
  if (Match(">", lex))  return MakeToken(TokenGt, start, 1);
  if (Match("[", lex))  return MakeToken(TokenLBracket, start, 1);
  if (Match("\\", lex)) return MakeToken(TokenBackslash, start, 1);
  if (Match("]", lex))  return MakeToken(TokenRBracket, start, 1);
  if (Match("^", lex))  return MakeToken(TokenCaret, start, 1);
  if (Match("{", lex))  return MakeToken(TokenLBrace, start, 1);
  if (Match("|", lex))  return MakeToken(TokenBar, start, 1);
  if (Match("}", lex))  return MakeToken(TokenRBrace, start, 1);
  if (Match("~", lex))  return MakeToken(TokenTilde, start, 1);

  return KeywordToken(lex);
}

static bool IsSymChar(char c)
{
  switch (c) {
  case '\0':
  case ' ':
  case '\t':
  case '\n':
  case '\r':
  case ';':
  case '(':
  case ')':
  case ',':
  case '.':
  case ':':
  case '[':
  case '\\':
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
  char *cur = SkipSpaces(lex->source + lex->pos);

  if (*cur == ';') cur = LineEnd(cur);

  lex->pos = cur - lex->source;
}

static bool Match(char *test, Lexer *lex)
{
  u32 start = lex->pos;
  while (*test != 0 && lex->source[lex->pos] != 0) {
    if (*test != lex->source[lex->pos]) {
      lex->pos = start;
      return false;
    }

    test++;
    lex->pos++;
  }
  return *test == 0;
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
  if (Match("0x", lex) && IsHexDigit(Peek(lex))) {
    while (Peek(lex) == '_' || IsHexDigit(Peek(lex))) lex->pos++;
    return MakeToken(TokenNum, lex->source + start, lex->pos - start);
  } else if (Match("$", lex)) {
    lex->pos++;
    return MakeToken(TokenNum, lex->source + start, lex->pos - start);
  } else {
    lex->pos = start;
  }

  if (Peek(lex) == '-') lex->pos++;
  while (Peek(lex) == '_' || IsDigit(Peek(lex))) lex->pos++;
  if (Peek(lex) == '.' && IsDigit(lex->source[lex->pos+1])) {
    lex->pos++;
    while (IsDigit(Peek(lex))) lex->pos++;
  }

  return MakeToken(TokenNum, lex->source + start, lex->pos - start);
}

static Token StringToken(Lexer *lex)
{
  u32 start = lex->pos;
  Match("\"", lex);
  while (Peek(lex) != '"') {
    if (Peek(lex) == '\\') lex->pos++;
    if (Peek(lex) == '\0') break;
    lex->pos++;
  }
  Match("\"", lex);
  return MakeToken(TokenStr, lex->source + start, lex->pos - start);
}

static Token KeywordToken(Lexer *lex)
{
  u32 start = lex->pos;

  switch (Peek(lex)) {
  case 'a':
    if (MatchKeyword("and", lex))     return MakeToken(TokenAnd, lex->source + start, lex->pos - start);
    if (MatchKeyword("as", lex))      return MakeToken(TokenAs, lex->source + start, lex->pos - start);
    break;
  case 'c':
    if (MatchKeyword("cond", lex))    return MakeToken(TokenCond, lex->source + start, lex->pos - start);
    break;
  case 'd':
    if (MatchKeyword("def", lex))     return MakeToken(TokenDef, lex->source + start, lex->pos - start);
    if (MatchKeyword("do", lex))      return MakeToken(TokenDo, lex->source + start, lex->pos - start);
    break;
  case 'e':
    if (MatchKeyword("else", lex))    return MakeToken(TokenElse, lex->source + start, lex->pos - start);
    if (MatchKeyword("end", lex))     return MakeToken(TokenEnd, lex->source + start, lex->pos - start);
    break;
  case 'f':
    if (MatchKeyword("false", lex))   return MakeToken(TokenFalse, lex->source + start, lex->pos - start);
    break;
  case 'i':
    if (MatchKeyword("if", lex))      return MakeToken(TokenIf, lex->source + start, lex->pos - start);
    if (MatchKeyword("import", lex))  return MakeToken(TokenImport, lex->source + start, lex->pos - start);
    if (MatchKeyword("in", lex))      return MakeToken(TokenIn, lex->source + start, lex->pos - start);
    break;
  case 'l':
    if (MatchKeyword("let", lex))     return MakeToken(TokenLet, lex->source + start, lex->pos - start);
    break;
  case 'm':
    if (MatchKeyword("module", lex))  return MakeToken(TokenModule, lex->source + start, lex->pos - start);
    break;
  case 'n':
    if (MatchKeyword("nil", lex))     return MakeToken(TokenNil, lex->source + start, lex->pos - start);
    if (MatchKeyword("not", lex))     return MakeToken(TokenNot, lex->source + start, lex->pos - start);
    break;
  case 'o':
    if (MatchKeyword("or", lex))      return MakeToken(TokenOr, lex->source + start, lex->pos - start);
    break;
  case 't':
    if (MatchKeyword("true", lex))    return MakeToken(TokenTrue, lex->source + start, lex->pos - start);
    break;
  }

  while (IsSymChar(Peek(lex))) lex->pos++;
  return MakeToken(TokenID, lex->source + start, lex->pos - start);
}
