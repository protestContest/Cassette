#include "value.h"
#include "mem.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "env.h"

int main(void)
{
  InitMem();

  char src[4096];
  FILE *file = fopen("test.rye", "r");
  u32 i = 0;
  i32 c;
  while ((c = fgetc(file)) != EOF) {
    src[i++] = c;
  }
  src[i] = '\0';
  printf("%s\n---\n", src);
  Val exp = Read(src);
  // printf("Exp: ");
  // PrintVal(exp);
  // printf("\n");
  PrintVal(Eval(exp, InitialEnv()));
  printf("\n");
}
