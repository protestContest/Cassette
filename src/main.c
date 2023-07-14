#include "run.h"

static void PrintUsage(void)
{
  Print("Usage:\n");
  Print("  cassette                Starts a REPL\n");
  Print("  cassette file.csst      Runs a script\n");
  Print("  cassette -c file.csst   Compiles a module\n");
  Print("  cassette -v             Prints the runtime version\n");
  Print("  cassette -h             Prints this help text\n");
}

i32 main(i32 argc, char **argv)
{
  Seed(Time());

  if (argc == 1) {
    REPL();
  } else if (argc == 2) {
    if (StrEq(argv[1], "-h")) {
      PrintUsage();
    } else if (StrEq(argv[1], "-v")) {
      PrintVersion();
    } else {
      RunFile(argv[1]);
    }
  } else if (argc == 3 && StrEq(argv[1], "-c")) {
    CompileFile(argv[2]);
  } else {
    PrintUsage();
  }
}
