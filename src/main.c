#include "mem.h"
#include "reader.h"
#include "value.h"

int main(void)
{
  InitMem();
  Read();

  printf("\n");
  // DumpAST(parsed);
  DumpPairs();
  DumpSymbols();
  DumpHeap();

  return 0;
}
