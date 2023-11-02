#include "project.h"
#include "source.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
  VM vm;

  char *sources[] = {"test.cst", "test2.cst"};
  Chunk *chunk = BuildProject(sources, ArrayCount(sources));

#ifdef DEBUG
  Disassemble(chunk);
#endif

  InitVM(&vm);
  RunChunk(chunk, &vm);

#ifdef DEBUG
  DumpMem(&vm.mem, &chunk->symbols);
#endif
}
