#include "compile.h"
#include "vm.h"
#include "primitives.h"
#include "univ.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
  char *source = ReadFile("./test.cst");
  Chunk chunk;
  Compiler c;
  VM vm;
  CompileResult result;
  char *error;

  InitChunk(&chunk);
  InitCompiler(&c, &chunk, Primitives(), NumPrimitives());

  result = Compile(source, &c);
  if (!result.ok) {
    PrintCompileError(result, &c);
    return 1;
  }

#ifdef DEBUG
  Disassemble(&chunk);
  printf("---\n");
#endif

  InitVM(&vm);
  error = RunChunk(&chunk, &vm);
  if (error) {
    printf("\nRuntime error: %s\n", error);
  }

#ifdef DEBUG
  DumpMem(&vm.mem, &chunk.symbols);
#endif
}
