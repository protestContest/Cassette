#include "compile.h"
#include <stdio.h>

int main(void)
{
  char *source = ReadFile("./test.cst");
  Chunk *chunk;

  printf("%s\n", source);
  printf("---\n");

  chunk = Compile(source);

  if (chunk) {
    Disassemble(chunk);
  }

  return 0;
}
