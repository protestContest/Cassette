#include "run.h"

i32 main(i32 argc, char **argv)
{
  if (argc == 1) {
    REPL();
  } else {
    char *filename = argv[1];
    RunFile(filename);
  }
}
