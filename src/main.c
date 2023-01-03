#include "value.h"
#include "mem.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "env.h"

int main(void)
{
  InitMem();
  PrintVal(Eval(ReadFile("prelude.rye"), InitialEnv()));
}
