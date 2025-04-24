#include "compile/lex.h"
#include "univ/str.h"

static Token MakeToken(TokenType type, u32 pos, u32 length)
{
  Token token;
  token.type = type;
  token.pos = pos;
  token.length = length;
  return token;
}

static bool IsSymChar(char ch)
{
  return IsAlpha(ch) || IsDigit(ch) || ch == '_' || ch == '!' || ch == '?';
}

static bool MatchKeyword(char *test, char *str)
{
  while (*test && *str) {
    if (*test != *str) return false;
    str++;
    test++;
  }
  if (*test) return false;
  return !IsSymChar(*str);
}

typedef struct {
  char *lexeme;
  TokenType type;
} Keyword;

static Keyword operators[] = {
  {"->",      arrowToken},
  {">>",      gtgtToken},
  {"<<",      ltltToken},
  {"<>",      ltgtToken},
  {"!=",      bangeqToken},
  {"==",      eqeqToken},
  {"<=",      lteqToken},
  {">=",      gteqToken},
  {"#",       hashToken},
  {"%",       percentToken},
  {"&",       ampToken},
  {"(",       lparenToken},
  {")",       rparenToken},
  {"*",       starToken},
  {"+",       plusToken},
  {",",       commaToken},
  {"-",       minusToken},
  {".",       dotToken},
  {"/",       slashToken},
  {":",       colonToken},
  {"<",       ltToken},
  {"=",       eqToken},
  {">",       gtToken},
  {"@",       atToken},
  {"[",       lbracketToken},
  {"\\",      bslashToken},
  {"]",       rbracketToken},
  {"^",       caretToken},
  {"{",       lbraceToken},
  {"|",       barToken},
  {"}",       rbraceToken},
  {"~",       tildeToken}
};

static Keyword keywords[] = {
  {"and",     andToken},
  {"as",      asToken},
  {"def",     defToken},
  {"do",      doToken},
  {"else",    elseToken},
  {"end",     endToken},
  {"false",   falseToken},
  {"guard",   guardToken},
  {"if",      ifToken},
  {"import",  importToken},
  {"in",      inToken},
  {"let",     letToken},
  {"module",  moduleToken},
  {"nil",     nilToken},
  {"not",     notToken},
  {"or",      orToken},
  {"record",  recordToken},
  {"true",    trueToken},
  {"when"  ,  whenToken},
};

Token NextToken(char *src, u32 pos)
{
  u32 i;

  if (src[pos] == ';') {
    while (src[pos] && !IsNewline(src[pos])) pos++;
  }

  if (Match("---\n", src+pos) && (pos == 0 || src[pos-1] == '\n')) {
    pos += 4;
    while (!Match("---\n", src+pos)) {
      pos++;
    }
    pos += 4;
  }

  if (src[pos] == 0) return MakeToken(eofToken, pos, 0);
  if (IsSpace(src[pos])) {
    u32 len = 0;
    while (IsSpace(src[pos+len])) len++;
    return MakeToken(spaceToken, pos, len);
  }
  if (IsNewline(src[pos])) return MakeToken(newlineToken, pos, 1);
  if (IsDigit(src[pos])) {
    if (src[pos+1] == 'x') {
      u32 len = 2;
      while (IsHexDigit(src[pos+len]) || src[pos+len] == '_') len++;
      return MakeToken(hexToken, pos, len);
    } else {
      u32 len = 1;
      while (IsDigit(src[pos+len]) || src[pos+len] == '_') len++;
      return MakeToken(numToken, pos, len);
    }
  }

  if (src[pos] == '"') {
    u32 len = 1;
    while (src[pos+len] && src[pos+len] != '"') {
      if (src[pos+len] == '\\') len++;
      len++;
    }
    if (src[pos+len] == 0) return MakeToken(errorToken, pos, len);
    return MakeToken(stringToken, pos, len+1);
  }
  if (src[pos] == '$') {
    if (src[pos+1] == 0) return MakeToken(errorToken, pos, 1);
    if (src[pos+1] == '\\') {
      if (src[pos+2] == 0) return MakeToken(errorToken, pos, 1);
      return MakeToken(byteToken, pos, 3);
    }
    return MakeToken(byteToken, pos, 2);
  }

  for (i = 0; i < ArrayCount(operators); i++) {
    if (Match(operators[i].lexeme, src+pos)) {
      return MakeToken(operators[i].type, pos, StrLen(operators[i].lexeme));
    }
  }

  for (i = 0; i < ArrayCount(keywords); i++) {
    if (MatchKeyword(keywords[i].lexeme, src+pos)) {
      return MakeToken(keywords[i].type, pos, StrLen(keywords[i].lexeme));
    }
  }

  if (IsSymChar(src[pos])) {
    u32 len = 0;
    while (IsSymChar(src[pos+len])) len++;
    return MakeToken(idToken, pos, len);
  }

  return MakeToken(errorToken, pos, 0);
}

bool IsKeyword(TokenType type)
{
  switch (type) {
  case andToken:
  case asToken:
  case defToken:
  case doToken:
  case elseToken:
  case endToken:
  case falseToken:
  case guardToken:
  case ifToken:
  case importToken:
  case inToken:
  case letToken:
  case moduleToken:
  case nilToken:
  case notToken:
  case orToken:
  case recordToken:
  case trueToken:
  case whenToken:
    return true;
  default:
    return false;
  }
}
