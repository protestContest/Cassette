#include "test.h"
#include "chunk.h"
#include "compile.h"
#include "mem.h"

void TestCompile(char *src, Chunk *expected)
{
  Chunk actual;
  InitChunk(&actual);
  Compile(src, &actual);

  if (!ChunksEqual(&actual, expected)) {
    Disassemble("Expected", expected);
    Disassemble("Actual", &actual);
  } else {
    printf(".");
  }
}
