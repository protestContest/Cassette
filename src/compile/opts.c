#include "compile/opts.h"
#include "univ/file.h"
#include "univ/str.h"
#include <unistd.h>

#define VERSION "2.0.0"

#define DEFAULT_IMPORTS "IO, List, Map, Math, Record, String, Value (nil?, integer?, symbol?, pair?, tuple?, binary?, error?, inspect), Host (typeof, symbol_name, format, hash)"

static void Usage(void)
{
  fprintf(stderr, "Usage: cassette [opts] script\n");
  fprintf(stderr, "  -v            Print version\n");
  fprintf(stderr, "  -c            Compile project\n");
  fprintf(stderr, "  -d            Enable debug mode\n");
  fprintf(stderr, "  -L lib_path   Library search path (defaults to $CASSETTE_PATH)\n");
  fprintf(stderr, "  -i imports    List of modules to auto-import\n");
}

/*
Search for an existing library path in this order:
- Env variable $CASSETTE_PATH
- $HOME/.local/share/cassette
- /usr/local/share/cassette
*/
static char *GetLibPath(void)
{
  char *path = getenv("CASSETTE_PATH");
  if (path && DirExists(path)) return NewString(path);
  path = StrCat(HomeDir(), "/.local/share/cassette");
  if (DirExists(path)) return path;
  free(path);
  path = "/usr/local/share/cassette";
  if (DirExists(path)) return NewString(path);
  return 0;
}

static void PrintVersion(void)
{
  char *lib_path = GetLibPath();
  printf("Cassette %s\n", VERSION);
  printf("Library path: %s\n", lib_path ? lib_path : "None");
  free(lib_path);
}

Opts *DefaultOpts(void)
{
  Opts *opts = malloc(sizeof(Opts));
  opts->debug = false;
  opts->compile = false;
  opts->lib_path = GetLibPath();
  opts->entry = 0;
  opts->default_imports = NewString(DEFAULT_IMPORTS);
  return opts;
}

Opts *ParseOpts(int argc, char *argv[])
{
  Opts *opts = DefaultOpts();
  int ch;

  while ((ch = getopt(argc, argv, "chvdL:i:")) >= 0) {
    switch (ch) {
    case 'c':
      opts->compile = true;
      break;
    case 'd':
      opts->debug = true;
      break;
    case 'L':
      free(opts->lib_path);
      opts->lib_path = NewString(optarg);
      break;
    case 'v':
      PrintVersion();
      FreeOpts(opts);
      return 0;
    case 'i':
      free(opts->default_imports);
      opts->default_imports = NewString(optarg);
      break;
    default:
      Usage();
      FreeOpts(opts);
      return 0;
    }
  }

  if (optind == argc) {
    Usage();
    FreeOpts(opts);
    exit(1);
  }

  opts->entry = argv[optind];
  return opts;
}

void FreeOpts(Opts *opts)
{
  if (opts->lib_path) free(opts->lib_path);
  if (opts->default_imports) free(opts->default_imports);
  free(opts);
}
