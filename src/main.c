#include "parse.h"
#include "node.h"
#include "types.h"
#include "primitives.h"
#include "compile.h"
#include "chunk.h"
#include "vm.h"
#include "error.h"
#include <univ/file.h>
#include <univ/str.h>
#include <univ/symbol.h>

int main(int argc, char *argv[])
{
  val result;
  char *source;

  if (argc < 2) {
    printf("%sSource files required%s\n", ANSIRed, ANSINormal);
    return 1;
  }

  InitMem(0);

  source = ReadFile(argv[1]);
  if (!source) {
    printf("%sCould not read file \"%s\"%s\n", ANSIRed, argv[1], ANSINormal);
    return 1;
  }

  printf("%s\n", argv[1]);

  printf("%s", source);
  if (source[strlen(source)-1] != '\n') printf("\n");
  printf("────────────────────────────────────────────────────────────\n");

  result = Parse(source);
  if (IsError(result)) {
    PrintError(PrefixError(result, "Parse error: "), source);
    return 1;
  }

  result = InferTypes(result);
  if (IsError(result)) {
    PrintError(PrefixError(result, "Type error: "), source);
    return 1;
  }
  PrintNode(result);

  result = Compile(result);
  if (IsError(result)) {
    PrintError(PrefixError(result, "Compile error: "), source);
    return 1;
  }

  PrintChunk(result);

  DestroyMem();
  free(source);
}
