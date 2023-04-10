#include "compile/parse.h"
#include "runtime/interpret.h"
#include "runtime/print.h"
#include "runtime/mem.h"
#include "univ/io.h"
#include <stdio.h>

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
