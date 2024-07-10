#include "parse.h"
#include "types.h"
#include "module.h"
#include "primitives.h"
#include "compile.h"
#include <univ/file.h>
#include <univ/str.h>
#include <univ/symbol.h>

int main(int argc, char *argv[])
{
  i32 i;
  i32 num_sources = argc-1;
  /*Module *modules = malloc(sizeof(Module)*num_sources);*/
  InitMem(0);

  if (num_sources < 1) {
    printf("%sSource files required%s\n", ANSIRed, ANSINormal);
    return 1;
  }

  for (i = 0; i < num_sources; i++) {
    char *source = ReadFile(argv[i+1]);
    /*Module *module = &modules[i];*/
    val result;
    if (!source) {
      printf("%sCould not read file \"%s\"%s\n", ANSIRed, argv[i+1], ANSINormal);
      return 1;
    }

    printf("%s\n", argv[i+1]);

    result = Parse(source);
    if (IsError(result)) {
      PrintError("Parse error", result, source);
      return 1;
    }

    PrintAST(result);

    result = InferTypes(result);
    if (IsError(result)) {
      PrintError("Parse error", result, source);
      return 1;
    }

    PrintAST(result);

/*
    InitModule(module);
    result = Compile(result, PrimitiveEnv(), module);
    if (IsError(result)) {
      PrintError("Compile error", result, source);
      return 1;
    }
*/
    free(source);
    printf("\n\n");
  }
}
