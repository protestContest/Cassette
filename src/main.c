#include "vm.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "env.h"
#include "proc.h"

int main(int argc, char **argv)
{
  VM vm;
  InitVM(&vm);

  Val env = InitialEnv(&vm);
  DefinePrimitives(&vm, env);

  char *filename = "test.rye";
  Val ast = ReadFile(&vm, filename);

  printf("> ");
  // PrintValue(&vm, ast);
  printf("\n\n");

  Val val = Eval(&vm, ast, env);
  PrintValue(&vm, val);
  printf("\n");

  return 0;
}
