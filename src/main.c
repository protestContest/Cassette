#include "module.h"
#include "parse.h"
#include "compile.h"
#include "result.h"
#include <univ.h>

int main(int argc, char *argv[])
{
  Result result;
  /*
  u8 *data = 0;
  FILE *file;
*/
  if (argc < 2) return 1;

  result = ParseFile(argv[1]);
  if (!result.ok) {
    PrintError(result);
    return 1;
  }

  PrintAST(result.value);

/*
  result = CompileModule(result.value);
  if (!IsOk(result)) {
    PrintError(result);
    return 1;
  }

  data = SerializeModule(result.value);

  HexDump(data, VecCount(data));

  remove("bin/out.wasm");
  file = fopen("bin/out.wasm", "w");
  fwrite(data, 1, VecCount(data), file);
  fclose(file);
*/
  return 0;
}
