#pragma once

/* The lexer produces tokens by scanning some source input. */

typedef enum {
  eofToken,
  newlineToken,
  spaceToken,
  bangeqToken,
  stringToken,
  hashToken,
  byteToken,
  percentToken,
  ampToken,
  lparenToken,
  rparenToken,
  starToken,
  plusToken,
  commaToken,
  minusToken,
  arrowToken,
  dotToken,
  slashToken,
  numToken,
  hexToken,
  colonToken,
  ltToken,
  ltltToken,
  lteqToken,
  ltgtToken,
  eqToken,
  eqeqToken,
  gtToken,
  gteqToken,
  gtgtToken,
  atToken,
  idToken,
  andToken,
  asToken,
  defToken,
  doToken,
  elseToken,
  endToken,
  exportToken,
  falseToken,
  guardToken,
  ifToken,
  importToken,
  inToken,
  letToken,
  moduleToken,
  nilToken,
  notToken,
  orToken,
  recordToken,
  trueToken,
  whenToken,
  lbraceToken,
  bslashToken,
  rbraceToken,
  caretToken,
  lbracketToken,
  barToken,
  rbracketToken,
  tildeToken,
  errorToken
} TokenType;

typedef struct {
  TokenType type;
  u32 pos;
  u32 length;
} Token;

Token NextToken(char *src, u32 pos);
bool IsKeyword(TokenType type);
