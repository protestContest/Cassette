#include "module.h"
#include "parse.h"
#include "compile.h"
#include "builderror.h"
#include <univ.h>

int main(int argc, char *argv[])
{
  Result result;
  u8 *data = 0;
  FILE *file;

  if (argc < 2) return 1;

  result = ParseFile(argv[1]);
  if (!IsOk(result)) {
    PrintError(ResError(result));
    return 1;
  }

  PrintAST(ResValue(result));

  result = CompileModule(ResValue(result));
  if (!IsOk(result)) {
    PrintError(ResError(result));
    return 1;
  }

  data = SerializeModule(ResValue(result));

  HexDump(data, VecCount(data));

  remove("bin/out.wasm");
  file = fopen("bin/out.wasm", "w");
  fwrite(data, 1, VecCount(data), file);
  fclose(file);

  return 0;
}
