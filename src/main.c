#include "parse.h"
#include "interpret.h"
#include "print.h"
#include "mem.h"
#include <univ/io.h>

int main(int argc, char *argv[])
{
  // char *src = "foo (1 + x) * 3 bar";
  char *src = "4.1 * (1 + ((2 - 3 * 8) / 2))";
  Print("Source: ");
  Print(src);
  Print("\n\n");

  ASTNode *ast = Parse(src);
  PrintAST(ast);
  Print("\n");

  Val *mem = NULL;
  InitMem(&mem);
  Val result = Interpret(ast, mem);

  Print("Result: ");
  PrintVal(mem, nil, result);
  Print("\n");
}
