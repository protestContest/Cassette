#include "parse.h"
#include "interpret.h"
#include "mem.h"
#include "ast.h"
#include "env.h"
#include "vm.h"

void REPL(Mem *mem)
{
  Val env = InitialEnv(mem);
  VM vm = {mem, NULL, env};
  char src[1024];

  while (true) {
    Print("> ");
    fgets(src, 1024, stdin);
    if (feof(stdin)) break;

    Val ast = Parse(src, mem);
    PrintVal(mem, Eval(ast, &vm));
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
