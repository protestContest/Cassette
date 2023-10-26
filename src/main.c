#include "compile.h"
#include "chunk.h"
#include <stdio.h>

int main(void)
{
  char *source = "(x) -> (x x) + 1 4\n73";
  Chunk *chunk = Compile(source);
  Assert(chunk);

  printf("%s\n", source);
  printf("---\n");
  printf("Constants: ");
  DumpConstants(chunk);
  printf("---\n");
  DumpChunk(chunk);
  printf("---\n");
  Disassemble(chunk);

  return 0;
}
