#include "args.h"

static u32 ParseShortArg(Opts *opts, char *flags, char *argv[])
{
  while (*flags != '\0') {
    switch (*flags++) {
    case 'c': opts->compile = true; break;
    case 'v': opts->version = true; break;
    case 'h': opts->help = true; break;
    case 't': opts->trace = true; break;
    case 'a': opts->print_ast = true; break;
    case 'm': opts->module_path = argv[0]; return 2;
    default: return false;
    }
  }

  return 1;
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
  Opts opts = {false, false, false, false, false, NULL, "."};

  for (u32 i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      u32 args_parsed = ParseShortArg(&opts, argv[i] + 1, argv + i + 1);
      if (args_parsed == 0) ArgError(argv[i]);
      else i += args_parsed - 1;
    } else if (opts.filename == NULL) {
      opts.filename = argv[i];
      if (StrEq(opts.module_path, ".")) {
        opts.module_path = FolderName(argv[i]);
      }
    } else {
      ArgError(argv[i]);
    }
  }

  return opts;
}
