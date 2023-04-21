#include "parse.h"
#include "interpret.h"
#include "mem.h"
#include "ast.h"
#include "env.h"
#include <stdio.h>

void REPL(Mem *mem)
{
  Val env = InitialEnv(mem);
  char src[1024];

  while (true) {
    Print("> ");
    fgets(src, 1024, stdin);
    if (feof(stdin)) break;

    Val ast = Parse(src, mem);
    PrintVal(mem, Eval(ast, env, mem));
    Print("\n");
  }
}

int main(int argc, char *argv[])
{
  Mem mem;
  InitMem(&mem, 0);

  if (argc > 1) {
    RunFile(argv[1], &mem);
  } else {
    REPL(&mem);
  }
}
