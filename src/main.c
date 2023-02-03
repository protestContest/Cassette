#include "chunk.h"
#include "compile.h"
#include "vm.h"

int main(int argc, char *argv[])
{
  Chunk chunk;
  InitChunk(&chunk);

  // char *src = "()";
  char *src = "def foo (x sum) -> if x = 0 sum (foo (x - 1) (sum + x)), foo 3 0";

  Compile(src, &chunk);
  Disassemble("Chunk", &chunk);

  VM vm;
  InitVM(&vm);
  RunChunk(&vm, &chunk);
}
