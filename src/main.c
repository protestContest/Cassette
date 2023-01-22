#include "module.h"
#include "repl.h"

int main(int argc, char *argv[])
{
  if (argc > 1) {
    RunProject(argv[1]);
  } else {
    Val env = RunProject(NULL);
    REPL(env);
  }
}
