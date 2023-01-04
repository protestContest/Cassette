#include "value.h"
#include "mem.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "env.h"

int main(void)
{
  InitMem();
  Val exp = ReadFile("test.rye");
  PrintVal(exp);
  PrintTree(exp);
  PrintVal(Eval(exp, InitialEnv()));
}
