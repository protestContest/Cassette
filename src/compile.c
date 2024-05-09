#include "compile.h"
#include "parse.h"

static CompileResult CompileOk = {true, 0, 0};

CompileResult Compile(i32 ast, Module *mod)
{
  return CompileOk;
}
