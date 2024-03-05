#include "univ/result.h"
#include "univ/vec.h"
#include "compile/symbol.h"
#include <stdio.h>
#include "compile/parse.h"
#include "compile/ast.h"
#include "compile/compile.h"
#include "compile/module.h"

int main(int argc, char *argv[])
{
  Result result;
  ByteVec mod;
  FILE *file;
  InitSymbols();

  if (argc < 2) return 1;

  result = ParseFile(argv[1]);
  if (!result.ok) return 1;

  PrintAST(ResultItem(result));

  result = CompileModule(ResultItem(result));
  if (!result.ok) {
    printf("%d: %s\n", ResultError(result)->pos, ResultError(result)->message);
    return 1;
  }

  mod = SerializeModule(ResultItem(result));
  remove("out.wasm");
  file = fopen("out.wasm", "w");
  fwrite(mod.items, 1, mod.count, file);
  fclose(file);

  return 0;
}
