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

void InitLexer(Lexer *lex, char *text, u32 start)
{
  lex->text = text;
  lex->pos = start;
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
  return IntVal(lex->token.pos);
}

static void Advance(Lexer *lex)
{
  if (IsEOF(lex)) return;
  lex->pos++;
}

static Token MakeToken(TokenType type, char *lexeme, u32 length, u32 pos)
{
  return (Token){type, lexeme, length, pos};
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
  default: return IsPrintable(Peek(lex));
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
  return MakeToken(TokenEOF, &Peek(lex), 0, lex->pos);
}

static Token NumberToken(Lexer *lex)
{
  char *start = &Peek(lex);
  u32 pos = lex->pos;

  if (Match(lex, "0x")) {
    while (IsHexDigit(lex)) Advance(lex);
    return MakeToken(TokenNum, start, &Peek(lex) - start, pos);
  }

  while (IsDigit(lex)) {
    Advance(lex);

    if (Peek(lex) == '.') {
      // float
      Advance(lex);

      while (!IsEOF(lex) && IsDigit(lex)) {
        Advance(lex);
      }

      return MakeToken(TokenNum, start, &Peek(lex) - start, pos);
    }
  }

  return MakeToken(TokenNum, start, &Peek(lex) - start, pos);
}

static Token StringToken(Lexer *lex, u32 pos)
{
  char *start = &Peek(lex);
  while (!IsEOF(lex) && Peek(lex) != '"') {
    if (Peek(lex) == '\\') Advance(lex);
    Advance(lex);
  }
  Token token = MakeToken(TokenString, start, &Peek(lex) - start, pos);
  Match(lex, "\"");
  return token;
}

static Token KeywordToken(Lexer *lex)
{
  char *start = &Peek(lex);
  u32 pos = lex->pos;
  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (MatchKeyword(lex, keywords[i].lexeme)) {
      return MakeToken(keywords[i].type, start, &Peek(lex) - start, pos);
    }
  }

  while (IsSymChar(lex)) Advance(lex);

  return MakeToken(TokenID, start, &Peek(lex) - start, pos);
}

static Token AdvanceToken(Lexer *lex)
{
  SkipWhitespace(lex);

  char *start = &Peek(lex);
  u32 pos = lex->pos;

  if (IsEOF(lex))       return EOFToken(lex);
  if (IsDigit(lex))     return NumberToken(lex);
  if (Match(lex, "\"")) return StringToken(lex, pos);

  if (IsNewline(lex)) {
    Advance(lex);
    return MakeToken(TokenNewline, start, 1, pos);
  }

  if (Match(lex, "!=")) return MakeToken(TokenBangEqual, start, &Peek(lex) - start, pos);
  if (Match(lex, "->")) return MakeToken(TokenArrow, start, &Peek(lex) - start, pos);
  if (Match(lex, "<=")) return MakeToken(TokenLessEqual, start, &Peek(lex) - start, pos);
  if (Match(lex, "==")) return MakeToken(TokenEqualEqual, start, &Peek(lex) - start, pos);
  if (Match(lex, ">=")) return MakeToken(TokenGreaterEqual, start, &Peek(lex) - start, pos);
  if (Match(lex, "#"))  return MakeToken(TokenHash, start, &Peek(lex) - start, pos);
  if (Match(lex, "%"))  return MakeToken(TokenPercent, start, &Peek(lex) - start, pos);
  if (Match(lex, "("))  return MakeToken(TokenLParen, start, &Peek(lex) - start, pos);
  if (Match(lex, ")"))  return MakeToken(TokenRParen, start, &Peek(lex) - start, pos);
  if (Match(lex, "*"))  return MakeToken(TokenStar, start, &Peek(lex) - start, pos);
  if (Match(lex, "+"))  return MakeToken(TokenPlus, start, &Peek(lex) - start, pos);
  if (Match(lex, ","))  return MakeToken(TokenComma, start, &Peek(lex) - start, pos);
  if (Match(lex, "-"))  return MakeToken(TokenMinus, start, &Peek(lex) - start, pos);
  if (Match(lex, "."))  return MakeToken(TokenDot, start, &Peek(lex) - start, pos);
  if (Match(lex, "/"))  return MakeToken(TokenSlash, start, &Peek(lex) - start, pos);
  if (Match(lex, ":"))  return MakeToken(TokenColon, start, &Peek(lex) - start, pos);
  if (Match(lex, "<"))  return MakeToken(TokenLess, start, &Peek(lex) - start, pos);
  if (Match(lex, "="))  return MakeToken(TokenEqual, start, &Peek(lex) - start, pos);
  if (Match(lex, ">"))  return MakeToken(TokenGreater, start, &Peek(lex) - start, pos);
  if (Match(lex, "["))  return MakeToken(TokenLBracket, start, &Peek(lex) - start, pos);
  if (Match(lex, "]"))  return MakeToken(TokenRBracket, start, &Peek(lex) - start, pos);
  if (Match(lex, "{"))  return MakeToken(TokenLBrace, start, &Peek(lex) - start, pos);
  if (Match(lex, "|"))  return MakeToken(TokenBar, start, &Peek(lex) - start, pos);
  if (Match(lex, "}"))  return MakeToken(TokenRBrace, start, &Peek(lex) - start, pos);

  return KeywordToken(lex);
}

void PrintToken(Token token)
{
  PrintN(token.lexeme, token.length);
}

void PrintTokenType(TokenType type)
{
  switch (type) {
  case TokenEOF:
    Print("TokenEOF\n");
    break;
  case TokenID:
    Print("TokenID\n");
    break;
  case TokenBangEqual:
    Print("TokenBangEqual\n");
    break;
  case TokenString:
    Print("TokenString\n");
    break;
  case TokenNewline:
    Print("TokenNewline\n");
    break;
  case TokenHash:
    Print("TokenHash\n");
    break;
  case TokenPercent:
    Print("TokenPercent\n");
    break;
  case TokenLParen:
    Print("TokenLParen\n");
    break;
  case TokenRParen:
    Print("TokenRParen\n");
    break;
  case TokenStar:
    Print("TokenStar\n");
    break;
  case TokenPlus:
    Print("TokenPlus\n");
    break;
  case TokenComma:
    Print("TokenComma\n");
    break;
  case TokenMinus:
    Print("TokenMinus\n");
    break;
  case TokenArrow:
    Print("TokenArrow\n");
    break;
  case TokenDot:
    Print("TokenDot\n");
    break;
  case TokenSlash:
    Print("TokenSlash\n");
    break;
  case TokenNum:
    Print("TokenNum\n");
    break;
  case TokenColon:
    Print("TokenColon\n");
    break;
  case TokenLess:
    Print("TokenLess\n");
    break;
  case TokenLessEqual:
    Print("TokenLessEqual\n");
    break;
  case TokenEqual:
    Print("TokenEqual\n");
    break;
  case TokenEqualEqual:
    Print("TokenEqualEqual\n");
    break;
  case TokenGreater:
    Print("TokenGreater\n");
    break;
  case TokenGreaterEqual:
    Print("TokenGreaterEqual\n");
    break;
  case TokenLBracket:
    Print("TokenLBracket\n");
    break;
  case TokenRBracket:
    Print("TokenRBracket\n");
    break;
  case TokenAnd:
    Print("TokenAnd\n");
    break;
  case TokenAs:
    Print("TokenAs\n");
    break;
  case TokenCond:
    Print("TokenCond\n");
    break;
  case TokenDef:
    Print("TokenDef\n");
    break;
  case TokenDo:
    Print("TokenDo\n");
    break;
  case TokenElse:
    Print("TokenElse\n");
    break;
  case TokenEnd:
    Print("TokenEnd\n");
    break;
  case TokenFalse:
    Print("TokenFalse\n");
    break;
  case TokenIf:
    Print("TokenIf\n");
    break;
  case TokenImport:
    Print("TokenImport\n");
    break;
  case TokenIn:
    Print("TokenIn\n");
    break;
  case TokenLet:
    Print("TokenLet\n");
    break;
  case TokenModule:
    Print("TokenModule\n");
    break;
  case TokenNil:
    Print("TokenNil\n");
    break;
  case TokenNot:
    Print("TokenNot\n");
    break;
  case TokenOr:
    Print("TokenOr\n");
    break;
  case TokenTrue:
    Print("TokenTrue\n");
    break;
  case TokenLBrace:
    Print("TokenLBrace\n");
    break;
  case TokenBar:
    Print("TokenBar\n");
    break;
  case TokenRBrace:
    Print("TokenRBrace\n");
    break;
  }
}
