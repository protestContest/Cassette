#include "rec.h"
#include "univ/file.h"

int main(int argc, char *argv[])
{
  Chunk *chunk;

  switch (argc) {
  case 1:
    chunk = LoadChunk(argv[1], NULL);
    break;
  case 2:
    chunk = LoadChunk(argv[1], argv[2]);
    break;
  default:
    return 1;
  }

  if (!chunk) return 1;

  VM vm;

  InitVM(&vm);
  RunChunk(&vm, chunk);
  DestroyVM(&vm);
  FreeChunk(chunk);
}
