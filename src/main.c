#include "result.h"
#include "univ/str.h"
#include "runtime/chunk.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "cli.h"
#include <stdio.h>

#ifdef DEBUG
#include "debug.h"
#endif

int main(int argc, char *argv[])
{
  Chunk chunk;
  Result result;
  VM vm;

  if (argc < 2) return Usage();

  InitChunk(&chunk);

  if (StrEq(argv[1], "-p")) {
    if (argc < 3) return Usage();
    result = BuildProject(argv[2], &chunk);
  } else {
    result = BuildScripts(argc - 1, argv + 1, &chunk);
  }

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
