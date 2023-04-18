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
  InitMem(&mem);

  char *src = ReadFile("test.rye");
  if (!src) {
    Print("Could not open file");
    Exit();
  }

  Val ast = Parse(src, &mem);
  PrintAST(ast, &mem);
  Interpret(ast, &mem);
}
