#include "vm.h"
#include "reader.h"
#include "value.h"

int main(void)
{
  InitVM();
  Read();

  printf("\n");
  // DumpAST(parsed);
  // DumpPairs();
  // DumpSymbols();
  // DumpHeap();

  return 0;
}
