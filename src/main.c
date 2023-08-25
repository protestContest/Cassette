#include "rec.h"

int main(int argc, char *argv[])
{
  Heap mem;
  InitMem(&mem, 0);

  ModuleResult result = LoadProject("test", "test/mod2.cst", &mem);
  if (!result.ok) {
    Print(result.error);
    Print("\n");
    return 1;
  }

  Chunk chunk;
  InitChunk(&chunk);
  Assemble(result.module.code, &chunk, &mem);

  Disassemble(&chunk);
  Print("\n");

  DestroyMem(&mem);
  InitMem(&mem, 0);

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);

  DestroyChunk(&chunk);
  DestroyVM(&vm);
  DestroyMem(&mem);
}
