#include "parse.h"
#include "node.h"
#include "types.h"
#include "module.h"
#include "primitives.h"
#include "compile.h"
#include "vm.h"
#include "error.h"
#include <univ/file.h>
#include <univ/str.h>
#include <univ/symbol.h>

int main(int argc, char *argv[])
{
  /*VM vm;*/
  i32 i;
  i32 num_sources = argc-1;
  Module *modules = malloc(sizeof(Module)*num_sources);
  val result;
  InitMem(0);

  if (num_sources < 1) {
    printf("%sSource files required%s\n", ANSIRed, ANSINormal);
    return 1;
  }

  for (i = 0; i < num_sources; i++) {
    char *source = ReadFile(argv[i+1]);
    Module *module = &modules[i];
    if (!source) {
      printf("%sCould not read file \"%s\"%s\n", ANSIRed, argv[i+1], ANSINormal);
      return 1;
    }

    printf("%s\n", argv[i+1]);

    printf("%s──────────────────────────────\n", source);

    result = Parse(source);
    if (IsError(result)) {
      PrintError(PrefixError(result, "Parse error: "), source);
      return 1;
    }
    result = InferTypes(result, PrimitiveTypes());
    if (IsError(result)) {
      PrintError(PrefixError(result, "Type error: "), source);
      return 1;
    }
    PrintNode(result);

    InitModule(module);
    result = Compile(result, PrimitiveEnv(), module);
    if (IsError(result)) {
      PrintError(PrefixError(result, "Compile error: "), source);
      return 1;
    }

    /*
    InitMem(256);
    InitVM(&vm, module);
    DefinePrimitives(&vm);
    while (!VMDone(&vm)) {
      VMTrace(&vm, source);
      VMStep(&vm);
    }
    if (vm.status != vmOk) {
      PrintError("Runtime error", VMError(&vm), source);
    }
    */

    free(source);
  }
}
