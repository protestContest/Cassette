#include "lex.h"

#define IsEOF(c)      ((c) == '\0')
#define IsSpace(c)    ((c) == ' ' || (c) == '\t')
#define IsNewline(c)  ((c) == '\n' || (c) == '\r')
#define IsDigit(c)    ((c) >= '0' && (c) <= '9')
#define Advance(src)  ((*(src))++)

static struct {char *lexeme; TokenType type;} keywords[] = {
  {"and",   TokenAnd },
  {"cond",  TokenCond },
  {"def",   TokenDef },
  {"do",    TokenDo },
  {"else",  TokenElse },
  {"end",   TokenEnd },
  {"false", TokenFalse },
  {"if",    TokenIf },
  {"in",    TokenIn },
  {"let",   TokenLet },
  {"nil",   TokenNil },
  {"not",   TokenNot },
  {"or",    TokenOr },
  {"true",  TokenTrue },
};

void SkipWhitespace(char **source)
{
  while (!IsEOF(**source) && IsSpace(**source)) Advance(source);

  if (**source == ';') {
    while (!IsEOF(**source) && !IsNewline(**source)) Advance(source);
    SkipWhitespace(source);
  }
}

bool IsSymChar(char c)
{
  if (IsEOF(c) || IsSpace(c) || IsNewline(c)) return false;

  switch (c) {
  case '#': return false;
  case ',': return false;
  case '.': return false;
  case '(': return false;
  case ')': return false;
  case '[': return false;
  case ']': return false;
  case '{': return false;
  case '}': return false;
  default: return true;
  }
}

bool MatchKeyword(char **source, char *keyword)
{
  char *start = *source;
  while (!IsEOF(**source) && *keyword != '\0') {
    if (**source != *keyword) {
      *source = start;
      return false;
    }

    Advance(source);
    keyword++;
  }

  if (IsSpace(**source) || IsNewline(**source) || IsEOF(**source)) {
    return true;
  }

  *source = start;
  return false;
}

bool Match(char **source, char *test)
{
  char *start = *source;
  while (!IsEOF(**source) && *test != '\0') {
    if (**source != *test) {
      *source = start;
      return false;
    }

    Advance(source);
    test++;
  }

  return true;
}

Token EOFToken(char **source)
{
  return (Token){TokenEOF, nil, *source, 0};
}

Token ErrorToken(char *message)
{
  return (Token){TokenError, nil, message, StrLen(message)};
}

Token NumberToken(char **source)
{
  char *start = *source;
  u32 whole = 0;
  while (!IsEOF(**source) && IsDigit(**source)) {
    u32 digit = (**source) - '0';
    whole *= 10;
    whole += digit;
    Advance(source);

    if (**source == '.') {
      // float
      Advance(source);
      if (!IsDigit(**source)) return ErrorToken("Expected digit after decimal point");

      float frac = 0;
      float place = 0.1;

      while (!IsEOF(**source) && IsDigit(**source)) {
        u32 digit = (**source) - '0';
        frac += digit * place;
        place /= 10;
        Advance(source);
      }

      return (Token){TokenNum, NumVal(whole + frac), start, *source - start};
    }
  }

  return (Token){TokenNum, IntVal(whole), start, *source - start};
}

Token StringToken(char **source)
{
  char *start = *source;
  while (!IsEOF(**source) && **source != '"') {
    Advance(source);
    if (**source == '\\') Advance(source);
  }
  Token token = {TokenString, nil, start, *source - start};
  Match(source, "\"");
  return token;
}

Token KeywordToken(char **source)
{
  char *start = *source;
  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (MatchKeyword(source, keywords[i].lexeme)) {
      return (Token){keywords[i].type, nil, start, *source - start};
    }
  }

  while (IsSymChar(**source)) Advance(source);

  return (Token){TokenID, nil, start, *source - start};
}

Token NextToken(char **source)
{
  SkipWhitespace(source);

  char *start = *source;

  if (IsEOF(**source)) return EOFToken(source);
  if (IsDigit(**source)) return NumberToken(source);
  if (Match(source, "\"")) return StringToken(source);

  if (IsNewline(**source)) {
    Advance(source);
    return (Token){TokenNewline, nil, start, 1};
  }

  if (Match(source, "->")) return (Token){TokenArrow, nil, start, *source - start};
  if (Match(source, "==")) return (Token){TokenEqualEqual, nil, start, *source - start};
  if (Match(source, "!=")) return (Token){TokenNotEqual, nil, start, *source - start};
  if (Match(source, ">=")) return (Token){TokenGreaterEqual, nil, start, *source - start};
  if (Match(source, "<=")) return (Token){TokenLessEqual, nil, start, *source - start};
  if (Match(source, "#[")) return (Token){TokenHashBracket, nil, start, *source - start};
  if (Match(source, ",")) return (Token){TokenComma, nil, start, *source - start};
  if (Match(source, "=")) return (Token){TokenEqual, nil, start, *source - start};
  if (Match(source, "(")) return (Token){TokenLParen, nil, start, *source - start};
  if (Match(source, ")")) return (Token){TokenRParen, nil, start, *source - start};
  if (Match(source, ">")) return (Token){TokenGreater, nil, start, *source - start};
  if (Match(source, "<")) return (Token){TokenLess, nil, start, *source - start};
  if (Match(source, "+")) return (Token){TokenPlus, nil, start, *source - start};
  if (Match(source, "-")) return (Token){TokenMinus, nil, start, *source - start};
  if (Match(source, "*")) return (Token){TokenStar, nil, start, *source - start};
  if (Match(source, "/")) return (Token){TokenSlash, nil, start, *source - start};
  if (Match(source, ":")) return (Token){TokenColon, nil, start, *source - start};
  if (Match(source, ".")) return (Token){TokenDot, nil, start, *source - start};
  if (Match(source, "[")) return (Token){TokenLBracket, nil, start, *source - start};
  if (Match(source, "]")) return (Token){TokenRBracket, nil, start, *source - start};
  if (Match(source, "{")) return (Token){TokenLBrace, nil, start, *source - start};
  if (Match(source, "}")) return (Token){TokenRBrace, nil, start, *source - start};

  return KeywordToken(source);
}

void PrintToken(Token token)
{
  switch (token.type) {
  case TokenEOF: Print("EOF"); break;
  case TokenError: Print("Error"); break;
  case TokenLet: Print("Let"); break;
  case TokenComma: Print("Comma"); break;
  case TokenEqual: Print("Equal"); break;
  case TokenDef: Print("Def"); break;
  case TokenLParen: Print("LParen"); break;
  case TokenRParen: Print("RParen"); break;
  case TokenID: Print("ID"); break;
  case TokenArrow: Print("Arrow"); break;
  case TokenAnd: Print("And"); break;
  case TokenOr: Print("Or"); break;
  case TokenEqualEqual: Print("EqualEqual"); break;
  case TokenNotEqual: Print("NotEqual"); break;
  case TokenGreater: Print("Greater"); break;
  case TokenGreaterEqual: Print("GreaterEqual"); break;
  case TokenLess: Print("Less"); break;
  case TokenLessEqual: Print("LessEqual"); break;
  case TokenIn: Print("In"); break;
  case TokenPlus: Print("Plus"); break;
  case TokenMinus: Print("Minus"); break;
  case TokenStar: Print("Star"); break;
  case TokenSlash: Print("Slash"); break;
  case TokenNot: Print("Not"); break;
  case TokenNum: Print("Num"); break;
  case TokenString: Print("String"); break;
  case TokenTrue: Print("True"); break;
  case TokenFalse: Print("False"); break;
  case TokenNil: Print("Nil"); break;
  case TokenColon: Print("Colon"); break;
  case TokenDot: Print("Dot"); break;
  case TokenDo: Print("Do"); break;
  case TokenEnd: Print("End"); break;
  case TokenIf: Print("If"); break;
  case TokenElse: Print("Else"); break;
  case TokenCond: Print("Cond"); break;
  case TokenLBracket: Print("LBracket"); break;
  case TokenRBracket: Print("RBracket"); break;
  case TokenHashBracket: Print("HashBracket"); break;
  case TokenLBrace: Print("LBrace"); break;
  case TokenRBrace: Print("RBrace"); break;
  case TokenNewline: Print("Newline"); break;
  }
}
