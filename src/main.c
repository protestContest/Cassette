#include "env.h"
#include "value.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"
#include "repl.h"

int ExecuteScript(char *path);

int main(int argc, char *argv[])
{
  if (argc > 1) {
    return ExecuteScript(argv[1]);
  } else {
    ExecuteScript("test.rye");
    // REPL();
  }
}

int ExecuteScript(char *path)
{
  Reader *r = NewReader();
  ReadFile(r, path);

  switch (r->status) {
  case PARSE_OK: {
    EvalResult result = Eval(r->ast, InitialEnv());
    if (result.status != EVAL_OK) {
      PrintEvalError(result);
    } else {
      PrintVal(result.value);
    }
    break;
  }
  case PARSE_INCOMPLETE:
    ParseError(r, "Unexpected end of input");
  case PARSE_ERROR:
    PrintReaderError(r);
  }
  return r->status;
}
