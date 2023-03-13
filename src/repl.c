#include "repl.h"
#include <univ/io.h>

void REPL(VM *vm)
{
  char line[1024];
  while (true) {
    Print("? ");
    if (!ReadLine(line, sizeof(line))) {
      Print("\n");
      break;
    }

    // Interpret(vm, line);
  }
}
