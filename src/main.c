#include "vm.h"
#include "reader.h"
#include "eval.h"
#include "env.h"
#include "proc.h"

int main(void)
{
  VM vm;
  InitVM(&vm);

  Val env = InitialEnv(&vm);
  DefinePrimitives(&vm, env);

  // Val sym = MakeSymbol(&vm, "a", 1);
  // Define(&vm, sym, IntVal(31), env);

  char *src = "(+ 3 1)";
  printf("> %s\n", src);

  Val ast = Read(&vm, src);

  Val val = Eval(&vm, ast, env);
  PrintValue(&vm, val);
  printf("\n");
  DumpVM(&vm);

  return 0;
}
