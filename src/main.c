#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/string.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  Chunk chunk;
  Result result;
  VM vm;

  if (argc < 2) {
    printf("Usage: cassette manifest.txt\n");
    return 1;
  }

  InitChunk(&chunk);
  result = BuildProject(argv[1], &chunk);

  if (!result.ok) {
    PrintError(result);
    DestroyChunk(&chunk);
    return 1;
  }

#ifdef DEBUG
  Disassemble(&chunk);
#endif

  InitVM(&vm);
  result = RunChunk(&chunk, &vm);
  if (!result.ok) {
    PrintError(result);
  }

  DestroyVM(&vm);
  DestroyChunk(&chunk);

  return 0;
}
