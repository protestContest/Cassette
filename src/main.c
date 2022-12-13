#include "mem.h"
#include "reader.h"
#include "symbol.h"

int main(void)
{
  InitMem(100);
  InitSymbols();
  Read();

  return 0;
}
