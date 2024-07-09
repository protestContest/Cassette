#include "parse.h"
#include "compile.h"
#include "primitives.h"
#include "types.h"
#include "vm.h"
#include <univ/file.h>
#include <univ/str.h>
#include <univ/vec.h>
#include <univ/symbol.h>
#include <univ/debug.h>

#define DEBUG 1

int main(int argc, char *argv[])
{
  VM vm;
  Module module;
  char *source = ReadFile(argv[1]);
  i32 result;

#if DEBUG
  printf("%sSource%s\n", ANSIUnderline, ANSINormal);
  printf("%s\n", source);
#endif

  result = Parse(source);

  if (IsError(result)) {
    PrintError("Parse error", result, source);
    return 1;
  }

  result = InferTypes(result);
  if (IsError(result)) {
    PrintError("Type error", result, source);
    return 1;
  }

#if DEBUG
  printf("%sAST%s\n", ANSIUnderline, ANSINormal);
  PrintAST(result);
  printf("\n");
#endif

  InitModule(&module);
  result = Compile(result, PrimitiveEnv(), &module);
  if (IsError(result)) {
    PrintError("Compile error", result, source);
    return 1;
  }

#if DEBUG
  {
    char *asm;
    printf("%sCompiled%s\n", ANSIUnderline, ANSINormal);
    HexDump(module.code, VecCount(module.code));
    printf("\n");
    printf("%sDisassembled%s\n", ANSIUnderline, ANSINormal);
    asm = Disassemble(&module);
    printf("%s\n", asm);
    FreeVec(asm);
  }
#endif

  InitMem(256);
  InitVM(&vm, &module);
  DefinePrimitives(&vm);

#if DEBUG
  printf("%sRuntime%s\n", ANSIUnderline, ANSINormal);
  while (!VMDone(&vm)) {
    VMTrace(&vm, source);
    VMStep(&vm);
  }
#else
  VMRun(&vm);
#endif

  if (vm.status != vmOk) {
    PrintError("Runtime error", VMError(&vm), source);
  }

  DestroyVM(&vm);
  DestroyModule(&module);
  DestroyMem();
  DestroySymbols();
  free(source);
}
