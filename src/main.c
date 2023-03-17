#include "lex.h"
#include <univ/io.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  char *src = "-0x3 * (4 + 0b101)";
  printf("\n%s\n\n", src);

  Lexer lex;
  InitLexer(&lex, src);

  Token token = NextToken(&lex);
  while (token.type != TOKEN_EOF) {
    PrintToken(token);
    printf("\n");
    token = NextToken(&lex);
  }
}
