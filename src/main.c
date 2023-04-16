#include "parse.h"
#include "interpret.h"
#include "mem.h"

int main(int argc, char *argv[])
{
  char *src;
  // src = "foo (1 + x) * 3 bar";
  // src = "4.1 * (1 + 2)\n3 - 2\n";
  // src = "(a b) -> (a - b) 1 4";
  src = ReadFile("test.rye");

  Print("Source: ");
  Print(src);
  Print("\n");

  Mem mem;
  InitMem(&mem);
  Val ast = Parse(src, &mem);

  Print("AST:\n");
  PrintAST(ast, &mem);
  Print("\n");

  // Val result = Interpret(ast, &mem);

  // Print("Result: ");
  // PrintVal(&mem, result);
  // Print("\n");
}
