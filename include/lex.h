#pragma once

typedef enum {
  eofToken,
  spaceToken,
  newlineToken,
  idToken,
  falseToken,
  trueToken,
  nilToken,
  stringToken,
  byteToken,
  numToken,
  hexToken,
  rbraceToken,
  lbraceToken,
  condToken,
  elseToken,
  ifToken,
  endToken,
  doToken,
  dotToken,
  colonToken,
  rbracketToken,
  lbracketToken,
  notToken,
  hashToken,
  tildeToken,
  slashToken,
  starToken,
  minusToken,
  plusToken,
  caretToken,
  ampToken,
  percentToken,
  gtgtToken,
  ltltToken,
  gtToken,
  ltToken,
  ltgtToken,
  barToken,
  bangeqToken,
  eqeqToken,
  orToken,
  andToken,
  arrowToken,
  bslashToken,
  eqToken,
  commaToken,
  letToken,
  rparenToken,
  lparenToken,
  defToken,
  asToken,
  importToken,
  exportsToken,
  moduleToken,
  errorToken
} TokenType;

typedef struct {
  TokenType type;
  u32 pos;
  u32 length;
} Token;

Token NextToken(char *src, u32 pos);
