#include "repl.h"
#include "platform/console.h"

void REPL(VM *vm, Image *image)
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
