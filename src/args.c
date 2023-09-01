#include "args.h"

void PrintUsage(void)
{
  Print("Usage: cassette script [options]\n");
  Print("\n");
  Print("  -h            Prints this message and exits\n");
  Print("  -p sources    Project directory contiaining scripts\n");
  Print("  -c            Compile project\n");
  Print("  -v            Verbose mode\n");
}

Args DefaultArgs(void)
{
  return (Args){NULL, ".", false, 0};
}

bool ParseArgs(int argc, char *argv[], Args *args)
{
  if (argc < 2) {
    PrintUsage();
    return false;
  }

  for (i32 i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      for (u32 j = 1; argv[i][j] != '\0'; j++) {
        switch (argv[i][j]) {
        case 'h':
          PrintUsage();
          return false;
        case 'p':
          if (i == argc - 1) {
            PrintUsage();
            return false;
          } else {
            args->dir = argv[i+1];
            i++;
          }
          break;
        case 'c':
          args->compile = true;
          break;
        case 'v':
          args->verbose++;
          break;
        default:
          PrintUsage();
          return false;
        }
      }
    } else {
      args->entry = argv[i];
    }
  }

  if (args->entry != NULL && args->dir != NULL) {
    args->entry = JoinPath(args->dir, args->entry);
  }

  return args->entry != NULL;
}
