#include "compile/parse.h"
#include <univ/io.h>

int main(int argc, char *argv[])
{
  char *src = "-3 * (4 + 1)";
  PrintLn(src);
  Parser p;
  InitParser(&p);
  Parse(&p, src);

  // Assert(p.error == NULL);

  // if (p.error != NULL) {
  //   PrintLn(p.error);
  //   PrintSourceContext(&p.source, p.token, 0);
  // } else {
  //   // PrintAST(ast);
  // }
}
