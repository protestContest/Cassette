#include "compile.h"
#include "vm.h"
#include <stdio.h>

int main(void)
{
  char *source = ReadFile("./test.cst");
  Chunk chunk;
  Compiler c;
  VM vm;
  CompileResult result;
  char *error;

  InitChunk(&chunk);
  InitCompiler(&c, &chunk);

  result = Compile(source, &c);
  if (!result.ok) {
    PrintCompileError(result, &c);
    return 1;
  }

  Disassemble(&chunk);
  printf("---\n");

  InitVM(&vm);
  error = RunChunk(&chunk, &vm);
  if (error) {
    printf("\nRuntime error: %s\n", error);
  } else {
    PrintVal(StackRef(&vm, 0), &chunk.symbols);
    printf("\n");
  }

  DumpMem(&vm.mem, &chunk.symbols);

  return 0;
}
