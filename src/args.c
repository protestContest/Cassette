#include "args.h"

static bool ParseShortArg(Opts *opts, char *flags)
{
  while (*flags != '\0') {
    switch (*flags++) {
    case 'c': opts->compile = true; break;
    case 'v': opts->version = true; break;
    case 'h': opts->help = true; break;
    case 't': opts->trace = true; break;
    default: return false;
    }
  }

  return true;
}

static void ArgError(char *arg)
{
  Print("Invalid argument: ");
  Print(arg);
  Print("\n");
  Exit();
}

Opts ParseArgs(u32 argc, char *argv[])
{
  Opts opts = {false, false, false, false, NULL};

  for (u32 i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (!ParseShortArg(&opts, argv[i] + 1)) {
        ArgError(argv[i]);
      }
    } else if (opts.filename == NULL) {
      opts.filename = argv[i];
    } else {
      ArgError(argv[i]);
    }
  }

  return opts;
}
