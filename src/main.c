#include <univ/io.h>
#include "image/module.h"
#include "runtime/vm.h"
#include "compile/compile.h"

int main(int argc, char *argv[])
{
  VM vm;
  InitVM(&vm);
  Image image;
  InitImage(&image);

  Assert(argc == 2);

  Print("Hello ");
  Print("World");
  Print("\n");

  char *src = ReadFile(argv[1]);
  Compile(src, &image);
  // RunImage(&vm, &image);
  FlushIO();
}
