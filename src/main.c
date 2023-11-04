#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/string.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  Chunk chunk;
  BuildResult result;
  VM vm;

  if (argc < 2) {
    printf("Usage: cassette manifest.txt\n");
    return 1;
  }

  InitChunk(&chunk);
  result = BuildProject(argv[1], &chunk);
  if (!result.ok) {
    PrintBuildError(result);
    return 1;
  }

  InitVM(&vm);
  RunChunk(&chunk, &vm);

  DestroyChunk(&chunk);
  DestroyVM(&vm);
  return 0;
}
