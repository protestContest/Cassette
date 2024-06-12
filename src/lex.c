#include "lex.h"
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

static bool Match(char *test, char *str)
{
  while (*test && *str) {
    if (*test != *str) return false;
    str++;
    test++;
  }
  return *test == 0;
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

Token NextToken(char *src, u32 pos)
{
  if (src[pos] == ';') {
    while (src[pos] && !IsNewline(src[pos])) pos++;
  }

  if (src[pos] == 0) return MakeToken(eofToken, pos, 0);
  if (IsSpace(src[pos])) {
    u32 len = 0;
    while (IsSpace(src[pos+len])) len++;
    return MakeToken(spaceToken, pos, len);
  }
  if (IsNewline(src[pos])) return MakeToken(newlineToken, pos, 1);
  if (IsDigit(src[pos])) {
    u32 len = 1;
    TokenType type = numToken;
    if (src[pos+len] == 'x') {
      len++;
      type = hexToken;
    }
    while (IsDigit(src[pos+len])) len++;
    return MakeToken(type, pos, len);
  }
  if (src[pos] == '"') {
    u32 len = 1;
    while (src[pos+len] && src[pos+len] != '"') len++;
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
  if (src[pos] == '[') return MakeToken(lbracketToken, pos, 1);
  if (src[pos] == ']') return MakeToken(rbracketToken, pos, 1);
  if (src[pos] == '{') return MakeToken(lbraceToken, pos, 1);
  if (src[pos] == '}') return MakeToken(rbraceToken, pos, 1);
  if (src[pos] == '(') return MakeToken(lparenToken, pos, 1);
  if (src[pos] == ')') return MakeToken(rparenToken, pos, 1);
  if (src[pos] == '.') return MakeToken(dotToken, pos, 1);
  if (src[pos] == ':') return MakeToken(colonToken, pos, 1);
  if (src[pos] == '#') return MakeToken(hashToken, pos, 1);
  if (src[pos] == '~') return MakeToken(tildeToken, pos, 1);
  if (src[pos] == '/') return MakeToken(slashToken, pos, 1);
  if (src[pos] == '*') return MakeToken(starToken, pos, 1);
  if (src[pos] == '-') return MakeToken(minusToken, pos, 1);
  if (src[pos] == '+') return MakeToken(plusToken, pos, 1);
  if (src[pos] == '>') return MakeToken(gtToken, pos, 1);
  if (src[pos] == '<') return MakeToken(ltToken, pos, 1);
  if (src[pos] == '|') return MakeToken(barToken, pos, 1);
  if (src[pos] == '\\') return MakeToken(bslashToken, pos, 1);
  if (src[pos] == '=') return MakeToken(eqToken, pos, 1);
  if (src[pos] == ',') return MakeToken(commaToken, pos, 1);
  if (src[pos] == '^') return MakeToken(caretToken, pos, 1);
  if (src[pos] == '&') return MakeToken(ampToken, pos, 1);
  if (src[pos] == '%') return MakeToken(percentToken, pos, 1);

  if (MatchKeyword("false", src+pos)) return MakeToken(falseToken, pos, 5);
  if (MatchKeyword("true", src+pos)) return MakeToken(trueToken, pos, 4);
  if (MatchKeyword("nil", src+pos)) return MakeToken(nilToken, pos, 3);
  if (MatchKeyword("cond", src+pos)) return MakeToken(condToken, pos, 4);
  if (MatchKeyword("else", src+pos)) return MakeToken(elseToken, pos, 4);
  if (MatchKeyword("if", src+pos)) return MakeToken(ifToken, pos, 2);
  if (MatchKeyword("end", src+pos)) return MakeToken(endToken, pos, 3);
  if (MatchKeyword("do", src+pos)) return MakeToken(doToken, pos, 2);
  if (MatchKeyword("or", src+pos)) return MakeToken(orToken, pos, 2);
  if (MatchKeyword("and", src+pos)) return MakeToken(andToken, pos, 3);
  if (MatchKeyword("let", src+pos)) return MakeToken(letToken, pos, 3);
  if (MatchKeyword("def", src+pos)) return MakeToken(defToken, pos, 3);
  if (MatchKeyword("as", src+pos)) return MakeToken(asToken, pos, 2);
  if (MatchKeyword("import", src+pos)) return MakeToken(importToken, pos, 6);
  if (MatchKeyword("exports", src+pos)) return MakeToken(exportsToken, pos, 7);
  if (MatchKeyword("module", src+pos)) return MakeToken(moduleToken, pos, 6);

  if (IsSymChar(src[pos])) {
    u32 len = 0;
    while (IsSymChar(src[pos+len])) len++;
    return MakeToken(idToken, pos, len);
  }

  return MakeToken(errorToken, pos, 0);
}
