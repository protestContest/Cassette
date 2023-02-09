#include "chunk.h"
#include "compile.h"
#include "vm.h"
#include "run.h"
#include "test.h"

int main(int argc, char *argv[])
{
  CompileTests();

  // VM vm;
  // InitVM(&vm);
  // Interpret(&vm, "def foo (x sum) -> if x = 0 sum (foo (x - 1) (sum + x))\nfoo 3 0");
  // Interpret(&vm, "def screen {width: 512, height: 342}\nscreen.width");

  // Chunk chunk;
  // InitChunk(&chunk);
  // Compile("def screen {width: 512, height: 342}\nscreen.width", &chunk);
  // Disassemble("Chunk", &chunk);

  printf("\n");
}
