#include "compile/parse.h"
#include <univ/io.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  char *src = "(a b) -> 3";
  // char *src = "foo";
  printf("\n%s\n\n", src);

  AST ast = Parse(src);
  PrintAST(ast);
}
