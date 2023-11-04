#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/string.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  Chunk chunk;
  Manifest manifest;
  Result result;
  VM vm;

  if (argc < 2) {
    printf("Usage: cassette manifest.txt\n");
    return 1;
  }

  InitManifest(&manifest);
  if (!ReadManifest(argv[1], &manifest)) {
    printf("Could not read manifest \"%s\"\n", argv[1]);
    DestroyManifest(&manifest);
    return 1;
  }

  InitChunk(&chunk);
  result = BuildProject(&manifest, &chunk);

  if (!result.ok) {
    PrintError(result);
    DestroyChunk(&chunk);
    DestroyManifest(&manifest);
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
  DestroyManifest(&manifest);

  return 0;
}
