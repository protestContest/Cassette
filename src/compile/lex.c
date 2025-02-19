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

static bool Match(char *test, char *str)
{
  while (*test && *str) {
    if (*test != *str) return false;
    str++;
    test++;
  }
  return *test == 0;
}

Token NextToken(char *src, u32 pos)
{
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
    return MakeToken(byteToken, pos, 2);
  }

  if (Match("->", src+pos)) return MakeToken(arrowToken, pos, 2);
  if (Match(">>", src+pos)) return MakeToken(gtgtToken, pos, 2);
  if (Match("<<", src+pos)) return MakeToken(ltltToken, pos, 2);
  if (Match("<>", src+pos)) return MakeToken(ltgtToken, pos, 2);
  if (Match("!=", src+pos)) return MakeToken(bangeqToken, pos, 2);
  if (Match("==", src+pos)) return MakeToken(eqeqToken, pos, 2);
  if (Match("<=", src+pos)) return MakeToken(lteqToken, pos, 2);
  if (Match(">=", src+pos)) return MakeToken(gteqToken, pos, 2);
  if (src[pos] == '#') return MakeToken(hashToken, pos, 1);
  if (src[pos] == '%') return MakeToken(percentToken, pos, 1);
  if (src[pos] == '&') return MakeToken(ampToken, pos, 1);
  if (src[pos] == '(') return MakeToken(lparenToken, pos, 1);
  if (src[pos] == ')') return MakeToken(rparenToken, pos, 1);
  if (src[pos] == '*') return MakeToken(starToken, pos, 1);
  if (src[pos] == '+') return MakeToken(plusToken, pos, 1);
  if (src[pos] == ',') return MakeToken(commaToken, pos, 1);
  if (src[pos] == '-') return MakeToken(minusToken, pos, 1);
  if (src[pos] == '.') return MakeToken(dotToken, pos, 1);
  if (src[pos] == '/') return MakeToken(slashToken, pos, 1);
  if (src[pos] == ':') return MakeToken(colonToken, pos, 1);
  if (src[pos] == '<') return MakeToken(ltToken, pos, 1);
  if (src[pos] == '=') return MakeToken(eqToken, pos, 1);
  if (src[pos] == '>') return MakeToken(gtToken, pos, 1);
  if (src[pos] == '@') return MakeToken(atToken, pos, 1);
  if (src[pos] == '[') return MakeToken(lbracketToken, pos, 1);
  if (src[pos] == '\\') return MakeToken(bslashToken, pos, 1);
  if (src[pos] == ']') return MakeToken(rbracketToken, pos, 1);
  if (src[pos] == '^') return MakeToken(caretToken, pos, 1);
  if (src[pos] == '{') return MakeToken(lbraceToken, pos, 1);
  if (src[pos] == '|') return MakeToken(barToken, pos, 1);
  if (src[pos] == '}') return MakeToken(rbraceToken, pos, 1);
  if (src[pos] == '~') return MakeToken(tildeToken, pos, 1);

  if (MatchKeyword("and", src+pos)) return MakeToken(andToken, pos, 3);
  if (MatchKeyword("as", src+pos)) return MakeToken(asToken, pos, 2);
  if (MatchKeyword("def", src+pos)) return MakeToken(defToken, pos, 3);
  if (MatchKeyword("do", src+pos)) return MakeToken(doToken, pos, 2);
  if (MatchKeyword("else", src+pos)) return MakeToken(elseToken, pos, 4);
  if (MatchKeyword("end", src+pos)) return MakeToken(endToken, pos, 3);
  if (MatchKeyword("export", src+pos)) return MakeToken(exportToken, pos, 7);
  if (MatchKeyword("false", src+pos)) return MakeToken(falseToken, pos, 5);
  if (MatchKeyword("guard", src+pos)) return MakeToken(guardToken, pos, 5);
  if (MatchKeyword("if", src+pos)) return MakeToken(ifToken, pos, 2);
  if (MatchKeyword("import", src+pos)) return MakeToken(importToken, pos, 6);
  if (MatchKeyword("in", src+pos)) return MakeToken(inToken, pos, 2);
  if (MatchKeyword("let", src+pos)) return MakeToken(letToken, pos, 3);
  if (MatchKeyword("module", src+pos)) return MakeToken(moduleToken, pos, 6);
  if (MatchKeyword("nil", src+pos)) return MakeToken(nilToken, pos, 3);
  if (MatchKeyword("not", src+pos)) return MakeToken(notToken, pos, 3);
  if (MatchKeyword("or", src+pos)) return MakeToken(orToken, pos, 2);
  if (MatchKeyword("record", src+pos)) return MakeToken(recordToken, pos, 6);
  if (MatchKeyword("true", src+pos)) return MakeToken(trueToken, pos, 4);
  if (MatchKeyword("when", src+pos)) return MakeToken(whenToken, pos, 4);

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
  case exportToken:
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
