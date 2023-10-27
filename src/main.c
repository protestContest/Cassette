#include "parse.h"
#include <stdio.h>

int main(void)
{
  /*char *source = "def (foo x) x + 1\nlet x = 1\nlet y = 2";*/
  char *source = ReadFile("./test.cst");
  Parser p;
  Val ast;

  InitParser(&p);
  ast = Parse(source, &p);

  if (ast == Error) {
    PrintParseError(&p);
  } else {
    PrintAST(ast, 0, &p.mem, &p.symbols);
  }

  return 0;
}
