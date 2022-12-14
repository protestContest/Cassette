#include "mem.h"
#include "reader.h"
#include "symbol.h"

int main(void)
{
  Input input = {NULL, NULL};

  InitMem();
  InitSymbols();
  Read(&input);

  return 0;
}
