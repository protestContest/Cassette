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

  // if (argc < 2) {
  //   Error("Not enough args");
  // }
  char *filename = "test.rye";
  Val ast = ReadFile(&vm, filename);

  printf("AST: ");
  PrintValue(&vm, ast);
  printf("\n");

  Val val = Eval(&vm, ast, env);
  PrintValue(&vm, val);
  printf("\n");
  DumpVM(&vm);

  return 0;
}
