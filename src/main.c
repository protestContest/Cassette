#include "args.h"
#include "run.h"

static void PrintUsage(void)
{
  Print("Usage:\n");
  Print("  cassette [-t]             Starts a REPL\n");
  Print("  cassette [-t] file.cst    Runs a script\n");
  Print("  cassette -c file.cst      Compiles a module\n");
  Print("  cassette -v               Prints the runtime version\n");
  Print("  cassette -h               Prints this help text\n");
  Print("\n");
  Print("Options:\n");
  Print("  -t            Enables instruction tracing\n");
}

i32 main(i32 argc, char **argv)
{
  Opts opts = ParseArgs(argc, argv);

  if (opts.version) PrintVersion();
  if (opts.help) {
    PrintUsage();
    return 0;
  }

  Seed(Time());

  if (opts.compile) {
    if (opts.filename == NULL) {
      PrintUsage();
      return 1;
    }

    CompileFile(opts.filename, opts.module_path);
  } else if (opts.filename != NULL) {
    RunFile(opts);
  } else {
    REPL(opts);
  }
}
