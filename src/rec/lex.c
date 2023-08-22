#include "lex.h"

#define Peek(lex)       ((lex)->text[(lex)->pos])
#define IsEOF(lex)      (Peek(lex) == '\0')
#define IsSpace(lex)    (Peek(lex) == ' '  || Peek(lex) == '\t')
#define IsNewline(lex)  (Peek(lex) == '\n' || Peek(lex) == '\r')
#define IsDigit(lex)    (Peek(lex) >= '0'  && Peek(lex) <= '9')
#define IsHexDigit(lex) (IsDigit(lex) || (Peek(lex) >= 'A'  && Peek(lex) <= 'F'))
#define IsUpper(lex)    (Peek(lex) >= 'A' && Peek(lex) <= 'Z')
#define IsLower(lex)    (Peek(lex) >= 'a' && Peek(lex) <= 'z')
#define IsAlpha(lex)    (IsUpper(lex) || IsLower(lex))

static struct {char *lexeme; TokenType type;} keywords[] = {
  {"and",     TokenAnd },
  {"as",      TokenAs },
  {"cond",    TokenCond },
  {"def",     TokenDef },
  {"do",      TokenDo },
  {"else",    TokenElse },
  {"end",     TokenEnd },
  {"false",   TokenFalse },
  {"if",      TokenIf },
  {"import",  TokenImport},
  {"in",      TokenIn },
  {"let",     TokenLet },
  {"module",  TokenModule},
  {"nil",     TokenNil },
  {"not",     TokenNot },
  {"or",      TokenOr },
  {"true",    TokenTrue },
};

static Token AdvanceToken(Lexer *lex);

void InitLexer(Lexer *lex, char *text)
{
  lex->text = text;
  lex->pos = 0;
  lex->token = AdvanceToken(lex);
}

Token NextToken(Lexer *lex)
{
  Token token = lex->token;
  lex->token = AdvanceToken(lex);
  return token;
}

Token PeekToken(Lexer *lex)
{
  return lex->token;
}

Val TokenPos(Lexer *lex)
{
  return IntVal(lex->token.lexeme - lex->text);
}

static void Advance(Lexer *lex)
{
  if (IsEOF(lex)) return;
  lex->pos++;
}

static Token MakeToken(TokenType type, char *lexeme, u32 length)
{
  return (Token){type, lexeme, length};
}

static void SkipWhitespace(Lexer *lex)
{
  while (!IsEOF(lex) && IsSpace(lex)) Advance(lex);

  if (Peek(lex) == ';') {
    while (!IsEOF(lex) && !IsNewline(lex)) Advance(lex);
    SkipWhitespace(lex);
  }
}

static bool IsSymChar(Lexer *lex)
{
  if (IsEOF(lex) || IsSpace(lex) || IsNewline(lex)) return false;

  switch (Peek(lex)) {
  case '"': return false;
  case '(': return false;
  case ')': return false;
  case '*': return false;
  case '+': return false;
  case ',': return false;
  case '-': return false;
  case '.': return false;
  case '/': return false;
  case ':': return false;
  case ';': return false;
  case '<': return false;
  case '=': return false;
  case '>': return false;
  case '[': return false;
  case ']': return false;
  case '{': return false;
  case '|': return false;
  case '}': return false;
  default: return true;
  }
}

static bool MatchKeyword(Lexer *lex, char *keyword)
{
  u32 start = lex->pos;
  while (*keyword != '\0') {
    if (IsEOF(lex)) return false;

    if (Peek(lex) != *keyword) {
      lex->pos = start;
      return false;
    }

    Advance(lex);
    keyword++;
  }

  if (!IsAlpha(lex)) return true;

  lex->pos = start;
  return false;
}

static bool Match(Lexer *lex, char *test)
{
  u32 start = lex->pos;
  while (!IsEOF(lex) && *test != '\0') {
    if (Peek(lex) != *test) {
      lex->pos = start;
      return false;
    }

    Advance(lex);
    test++;
  }

  return true;
}

static Token EOFToken(Lexer *lex)
{
  return MakeToken(TokenEOF, &Peek(lex), 0);
}

static Token NumberToken(Lexer *lex)
{
  char *start = &Peek(lex);

  if (Match(lex, "0x")) {
    while (IsHexDigit(lex)) Advance(lex);
    return MakeToken(TokenNum, start, &Peek(lex) - start);
  }

  while (IsDigit(lex)) {
    Advance(lex);

    if (Peek(lex) == '.') {
      // float
      Advance(lex);

      while (!IsEOF(lex) && IsDigit(lex)) {
        Advance(lex);
      }

      return MakeToken(TokenNum, start, &Peek(lex) - start);
    }
  }

  return MakeToken(TokenNum, start, &Peek(lex) - start);
}

static Token StringToken(Lexer *lex)
{
  char *start = &Peek(lex);
  while (!IsEOF(lex) && Peek(lex) != '"') {
    if (Peek(lex) == '\\') Advance(lex);
    Advance(lex);
  }
  Token token = MakeToken(TokenString, start, &Peek(lex) - start);
  Match(lex, "\"");
  return token;
}

static Token KeywordToken(Lexer *lex)
{
  char *start = &Peek(lex);
  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (MatchKeyword(lex, keywords[i].lexeme)) {
      return MakeToken(keywords[i].type, start, &Peek(lex) - start);
    }
  }

  while (IsSymChar(lex)) Advance(lex);

  return MakeToken(TokenID, start, &Peek(lex) - start);
}

static Token AdvanceToken(Lexer *lex)
{
  SkipWhitespace(lex);

  char *start = &Peek(lex);

  if (IsEOF(lex))       return EOFToken(lex);
  if (IsDigit(lex))     return NumberToken(lex);
  if (Match(lex, "\"")) return StringToken(lex);

  if (IsNewline(lex)) {
    Advance(lex);
    return MakeToken(TokenNewline, start, 1);
  }

  if (Match(lex, "!=")) return MakeToken(TokenBangEqual, start, &Peek(lex) - start);
  if (Match(lex, "->")) return MakeToken(TokenArrow, start, &Peek(lex) - start);
  if (Match(lex, "<=")) return MakeToken(TokenLessEqual, start, &Peek(lex) - start);
  if (Match(lex, "==")) return MakeToken(TokenEqualEqual, start, &Peek(lex) - start);
  if (Match(lex, ">=")) return MakeToken(TokenGreaterEqual, start, &Peek(lex) - start);
  if (Match(lex, "#"))  return MakeToken(TokenHash, start, &Peek(lex) - start);
  if (Match(lex, "%"))  return MakeToken(TokenPercent, start, &Peek(lex) - start);
  if (Match(lex, "("))  return MakeToken(TokenLParen, start, &Peek(lex) - start);
  if (Match(lex, ")"))  return MakeToken(TokenRParen, start, &Peek(lex) - start);
  if (Match(lex, "*"))  return MakeToken(TokenStar, start, &Peek(lex) - start);
  if (Match(lex, "+"))  return MakeToken(TokenPlus, start, &Peek(lex) - start);
  if (Match(lex, ","))  return MakeToken(TokenComma, start, &Peek(lex) - start);
  if (Match(lex, "-"))  return MakeToken(TokenMinus, start, &Peek(lex) - start);
  if (Match(lex, "."))  return MakeToken(TokenDot, start, &Peek(lex) - start);
  if (Match(lex, "/"))  return MakeToken(TokenSlash, start, &Peek(lex) - start);
  if (Match(lex, ":"))  return MakeToken(TokenColon, start, &Peek(lex) - start);
  if (Match(lex, "<"))  return MakeToken(TokenLess, start, &Peek(lex) - start);
  if (Match(lex, "="))  return MakeToken(TokenEqual, start, &Peek(lex) - start);
  if (Match(lex, ">"))  return MakeToken(TokenGreater, start, &Peek(lex) - start);
  if (Match(lex, "["))  return MakeToken(TokenLBracket, start, &Peek(lex) - start);
  if (Match(lex, "]"))  return MakeToken(TokenRBracket, start, &Peek(lex) - start);
  if (Match(lex, "{"))  return MakeToken(TokenLBrace, start, &Peek(lex) - start);
  if (Match(lex, "|"))  return MakeToken(TokenBar, start, &Peek(lex) - start);
  if (Match(lex, "}"))  return MakeToken(TokenRBrace, start, &Peek(lex) - start);

  return KeywordToken(lex);
}