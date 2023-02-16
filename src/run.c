#include "run.h"
#include "module.h"
#include "compile.h"

void REPL(VM *vm)
{
  char line[1024];
  while (true) {
    printf("? ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    Interpret(vm, line);
  }
}

void RunFile(VM *vm, char *path)
{
  Chunk chunk;
  InitChunk(&chunk);

  // LoadModules(".", path, &chunk);

  char *src = ReadFile(path);
  Compile(src, &chunk);
  RunChunk(vm, &chunk);

  free(src);
}