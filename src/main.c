#include "vm.h"
#include "reader.h"
#include "value.h"

int main(void)
{
  InitVM();
  Value ast = Read();

  printf("\n");
  DumpAST(ast);
  DumpPairs();
  // DumpSymbols();
  // DumpHeap();

  return 0;
}
