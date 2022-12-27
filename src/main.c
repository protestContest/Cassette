#include "vm.h"
#include "reader.h"

int main(void)
{
  VM vm;
  InitVM(&vm);

  char *src = "ab 12 3.14 ";
  printf("> %s\n", src);
  Val val = Read(&vm, src);
  PrintValue(&vm, val);
  printf("\n");

  DumpVM(&vm);

  return 0;
}
