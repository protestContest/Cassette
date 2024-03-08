#include "ast.h"
#include "module.h"
#include "parse.h"
#include "compile.h"
#include "builderror.h"
#include <univ.h>

int main(int argc, char *argv[])
{
  Result result;
  Node *ast;
  Module *mod;
  u8 *data = 0;
  FILE *file;

  if (argc < 2) return 1;

  result = ParseFile(argv[1]);
  if (!IsOk(result)) return 1;
  ast = ResValue(result);

  PrintAST(ast);

  result = CompileModule(ast);
  if (!IsOk(result)) {
    BuildError *err = ResError(result);
    char *source = ReadFile(argv[1]);
    u32 line = LineNum(source, err->pos);
    u32 col = ColNum(source, err->pos);

    printf("%s:%d:%d: %s\n", err->filename, line, col, err->message);
    return 1;
  }

  mod = ResValue(result);
  data = SerializeModule(mod);

  HexDump(data, VecCount(data));

  remove("bin/out.wasm");
  file = fopen("bin/out.wasm", "w");
  fwrite(data, 1, VecCount(data), file);
  fclose(file);

  return 0;
}
