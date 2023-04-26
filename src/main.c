#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "eval.h"
#include <univ/window.h>

void REPL(Mem *mem)
{
  char line[1024];
  Source src = {"repl", line, 0};

  VM vm;
  InitVM(&vm, mem);

  while (true) {
    Print("> ");
    if (!ReadLine(line, 1024)) break;
    src.length = StrLen(line);

    Val ast = Parse(src, mem);
    PrintVal(mem, Eval(ast, &vm));
    Print("\n");

    HandleEvents();
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
