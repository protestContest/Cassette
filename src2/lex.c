#include "lex.h"

#define Peek(lex)       ((lex)->text[(lex)->pos])
#define IsEOF(lex)      (Peek(lex) == '\0')
#define IsSpace(lex)    (Peek(lex) == ' '  || Peek(lex) == '\t')
#define IsNewline(lex)  (Peek(lex) == '\n' || Peek(lex) == '\r')
#define IsDigit(lex)    (Peek(lex) >= '0'  && Peek(lex) <= '9')

static struct {char *lexeme; TokenType type;} keywords[] = {
  {"and",     TokenAnd },
  {"cond",    TokenCond },
  {"def",     TokenDef },
  {"do",      TokenDo },
  {"else",    TokenElse },
  {"end",     TokenEnd },
  {"false",   TokenFalse },
  {"if",      TokenIf },
  {"in",      TokenIn },
  {"let",     TokenLet },
  {"nil",     TokenNil },
  {"not",     TokenNot },
  {"or",      TokenOr },
  {"true",    TokenTrue },
  {"module",  TokenModule},
};

static Token AdvanceToken(Lexer *lex);

void InitLexer(Lexer *lex, char *text)
{
  lex->text = text;
  lex->pos = 0;
  lex->line = 1;
  lex->col = 1;
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

static void Advance(Lexer *lex)
{
  if (IsEOF(lex)) return;

  if (IsNewline(lex)) {
    lex->col = 1;
    lex->line++;
  } else {
    lex->col++;
  }
  lex->pos++;
}

Token MakeToken(TokenType type, char *lexeme, u32 length, u32 line, u32 col)
{
  return (Token){type, lexeme, length, line, col};
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
  case '#': return false;
  case ',': return false;
  case '.': return false;
  case '|': return false;
  case ':': return false;
  case '(': return false;
  case ')': return false;
  case '[': return false;
  case ']': return false;
  case '{': return false;
  case '}': return false;
  default: return true;
  }
}

static bool MatchKeyword(Lexer *lex, char *keyword)
{
  u32 start = lex->pos;
  while (!IsEOF(lex) && *keyword != '\0') {
    if (Peek(lex) != *keyword) {
      lex->pos = start;
      return false;
    }

    Advance(lex);
    keyword++;
  }

  if (IsSpace(lex) || IsNewline(lex) || IsEOF(lex)) {
    return true;
  }

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
  return MakeToken(TokenEOF, &Peek(lex), 0, lex->line, lex->col);
}

static Token ErrorToken(char *message, Lexer *lex)
{
  return MakeToken(TokenError, message, StrLen(message), lex->line, lex->col);
}

static Token NumberToken(Lexer *lex)
{
  char *start = &Peek(lex);
  u32 line = lex->line;
  u32 col = lex->col;

  while (!IsEOF(lex) && IsDigit(lex)) {
    Advance(lex);

    if (Peek(lex) == '.') {
      // float
      Advance(lex);
      if (!IsDigit(lex)) return ErrorToken("Expected digit after decimal point", lex);

      while (!IsEOF(lex) && IsDigit(lex)) {
        Advance(lex);
      }

      return MakeToken(TokenNum, start, &Peek(lex) - start, line, col);
    }
  }

  return MakeToken(TokenNum, start, &Peek(lex) - start, line, col);
}

static Token StringToken(Lexer *lex)
{
  char *start = &Peek(lex);
  u32 line = lex->line;
  u32 col = lex->col;
  while (!IsEOF(lex) && Peek(lex) != '"') {
    Advance(lex);
    if (Peek(lex) == '\\') Advance(lex);
  }
  Token token = MakeToken(TokenString, start, &Peek(lex) - start, line, col);
  Match(lex, "\"");
  return token;
}

static Token KeywordToken(Lexer *lex)
{
  char *start = &Peek(lex);
  u32 line = lex->line;
  u32 col = lex->col;
  for (u32 i = 0; i < ArrayCount(keywords); i++) {
    if (MatchKeyword(lex, keywords[i].lexeme)) {
      return MakeToken(keywords[i].type, start, &Peek(lex) - start, line, col);
    }
  }

  while (IsSymChar(lex)) Advance(lex);

  return MakeToken(TokenID, start, &Peek(lex) - start, line, col);
}

static Token AdvanceToken(Lexer *lex)
{
  SkipWhitespace(lex);

  char *start = &Peek(lex);
  u32 line = lex->line;
  u32 col = lex->col;

  if (IsEOF(lex)) return EOFToken(lex);
  if (IsDigit(lex)) return NumberToken(lex);
  if (Match(lex, "\"")) return StringToken(lex);

  if (IsNewline(lex)) {
    Advance(lex);
    return MakeToken(TokenNewline, start, 1, line, col);
  }

  if (Match(lex, "->")) return MakeToken(TokenArrow, start, &Peek(lex) - start, line, col);
  if (Match(lex, "==")) return MakeToken(TokenEqualEqual, start, &Peek(lex) - start, line, col);
  if (Match(lex, "!=")) return MakeToken(TokenNotEqual, start, &Peek(lex) - start, line, col);
  if (Match(lex, ">=")) return MakeToken(TokenGreaterEqual, start, &Peek(lex) - start, line, col);
  if (Match(lex, "<=")) return MakeToken(TokenLessEqual, start, &Peek(lex) - start, line, col);
  if (Match(lex, "#[")) return MakeToken(TokenHashBracket, start, &Peek(lex) - start, line, col);
  if (Match(lex, "|")) return MakeToken(TokenPipe, start, &Peek(lex) - start, line, col);
  if (Match(lex, ",")) return MakeToken(TokenComma, start, &Peek(lex) - start, line, col);
  if (Match(lex, "=")) return MakeToken(TokenEqual, start, &Peek(lex) - start, line, col);
  if (Match(lex, "(")) return MakeToken(TokenLParen, start, &Peek(lex) - start, line, col);
  if (Match(lex, ")")) return MakeToken(TokenRParen, start, &Peek(lex) - start, line, col);
  if (Match(lex, ">")) return MakeToken(TokenGreater, start, &Peek(lex) - start, line, col);
  if (Match(lex, "<")) return MakeToken(TokenLess, start, &Peek(lex) - start, line, col);
  if (Match(lex, "+")) return MakeToken(TokenPlus, start, &Peek(lex) - start, line, col);
  if (Match(lex, "-")) return MakeToken(TokenMinus, start, &Peek(lex) - start, line, col);
  if (Match(lex, "*")) return MakeToken(TokenStar, start, &Peek(lex) - start, line, col);
  if (Match(lex, "/")) return MakeToken(TokenSlash, start, &Peek(lex) - start, line, col);
  if (Match(lex, ":")) return MakeToken(TokenColon, start, &Peek(lex) - start, line, col);
  if (Match(lex, ".")) return MakeToken(TokenDot, start, &Peek(lex) - start, line, col);
  if (Match(lex, "[")) return MakeToken(TokenLBracket, start, &Peek(lex) - start, line, col);
  if (Match(lex, "]")) return MakeToken(TokenRBracket, start, &Peek(lex) - start, line, col);
  if (Match(lex, "{")) return MakeToken(TokenLBrace, start, &Peek(lex) - start, line, col);
  if (Match(lex, "}")) return MakeToken(TokenRBrace, start, &Peek(lex) - start, line, col);

  return KeywordToken(lex);
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
  case TokenPipe: Print("Pipe"); break;
  case TokenNewline: Print("Newline"); break;
  case TokenModule: Print("Module"); break;
  }
}
