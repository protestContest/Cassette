#include "env.h"
#include "value.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"

void REPL(void);

int main(int argc, char *argv[])
{
  if (argc > 1) {
    Val exp = ReadFile(argv[1]);
    Eval(exp);
  } else {
    REPL();
  }
}

bool GetLine(char *buf)
{
  printf("? ");
  fgets(buf, 1024, stdin);
  if (feof(stdin)) return false;
  return true;
}

void REPL(void)
{
  char buf[1024];
  Val env = InitialEnv();
  while (GetLine(buf)) {
    Val exp = Read(buf);
    PrintVal(EvalIn(exp, env));
  }
}
