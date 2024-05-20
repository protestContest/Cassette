#include "parse.h"
#include "compile.h"
#include "primitives.h"
#include "vm.h"
#include <univ.h>

int main(int argc, char *argv[])
{
  VM vm;
  Module module;
  char *source = ReadFile(argv[1]);
  char *asm;
  i32 length, result;

  printf("%sSource%s\n", ANSIUnderline, ANSINormal);
  printf("%s\n", source);
  length = strlen(source);
  result = Parse(source, length);

  if (IsError(result)) {
    PrintError(result, source);
    return 1;
  }

  printf("%sAST%s\n", ANSIUnderline, ANSINormal);
  PrintAST(result);
  printf("\n");

  InitModule(&module);
  result = Compile(result, PrimitiveEnv(), &module);
  if (IsError(result)) {
    PrintError(result, source);
    return 1;
  }

  printf("%sCompiled%s\n", ANSIUnderline, ANSINormal);
  HexDump(module.code, VecCount(module.code));
  printf("\n");
  printf("%sDisassembled%s\n", ANSIUnderline, ANSINormal);
  asm = Disassemble(&module);
  printf("%s\n", asm);
  FreeVec(asm);

  InitMem(256);
  InitVM(&vm, &module);
  DefinePrimitives(&vm);

  printf("%sRuntime%s\n", ANSIUnderline, ANSINormal);
  while (!VMDone(&vm)) {
    VMTrace(&vm, source);
    VMStep(&vm);
  }

  if (vm.status != vmOk) {
    PrintError(VMError(&vm), source);
  }

  DestroyVM(&vm);
  DestroyModule(&module);
  DestroyMem();
  DestroySymbols();
  free(source);
}
