#include "opts.h"
#include <stdio.h>

#ifndef LIBCASSETTE

void PrintUsage(void)
{
  Print("Usage: cassette script [options]\n");
  Print("\n");
  Print("  -h            Prints this message and exits\n");
  Print("  -p sources    Project directory contiaining scripts\n");
  Print("  -c            Compile project\n");
  Print("  -v            Verbose mode\n");
  Print("  -s seed       Set random seed\n");
}

CassetteOpts DefaultOpts(void)
{
  return (CassetteOpts){NULL, ".", false, 0, Microtime()};
}

bool ParseArgs(int argc, char *argv[], CassetteOpts *args)
{
  if (argc < 2) {
    PrintUsage();
    return false;
  }

  for (i32 i = 1; i < argc; i++) {
    bool skip_next = false;
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
            skip_next = true;
          }
          break;
        case 'c':
          args->compile = true;
          break;
        case 'v':
          args->verbose++;
          break;
        case 's':
          if (i == argc - 1) {
            PrintUsage();
            return false;
          } else {
            sscanf(argv[i+1], "%u", &args->seed);
            skip_next = true;
          }
          break;
        default:
          PrintUsage();
          return false;
        }
      }
    } else {
      args->entry = argv[i];
    }

    if (skip_next) i++;
  }

  if (args->entry != NULL && args->dir != NULL) {
    args->entry = JoinPath(args->dir, args->entry);
  }

  return args->entry != NULL;
}

#endif
