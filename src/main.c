#include "mem.h"
#include "reader.h"

int main(void)
{
  InitMem();

  Input input = {NULL, NULL};
  Read(&input);

  return 0;
}
