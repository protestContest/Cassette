#include "vm.h"
#include "parse.h"
#include "eval.h"
#include <univ/window.h>

void REPL(Mem *mem)
{
  VM vm;
  InitVM(&vm, mem);

  char src[1024];

  while (true) {
    Print("> ");
    if (!ReadLine(src, 1024)) break;

    Val ast = Parse(src, StrLen(src), mem);
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
