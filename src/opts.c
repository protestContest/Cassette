#include "opts.h"
#include "univ/file.h"
#include "univ/str.h"
#include <unistd.h>

#define VERSION "2.0.0"

#define DEFAULT_IMPORTS "Enum, IO, List, Map, Math, Record, String, Value (nil?, integer?, symbol?, pair?, tuple?, binary?, error?, inspect), Host (typeof, symbol_name, format, hash)"

static void Usage(void)
{
  fprintf(stderr, "Usage: cassette [opts] script\n");
  fprintf(stderr, "  -v            Print version\n");
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

bool ParseOpts(int argc, char *argv[], Opts *opts)
{
  int ch;
  opts->debug = false;
  opts->lib_path = 0;
  opts->entry = 0;
  opts->default_imports = 0;

  while ((ch = getopt(argc, argv, "hvdL:i:")) >= 0) {
    switch (ch) {
    case 'd':
      opts->debug = true;
      break;
    case 'L':
      opts->lib_path = NewString(optarg);
      break;
    case 'v':
      PrintVersion();
      return false;
    case 'i':
      opts->default_imports = NewString(optarg);
      break;
    default:
      Usage();
      return false;
    }
  }

  if (optind == argc) {
    Usage();
    exit(1);
  }

  if (opts->lib_path == 0) {
    opts->lib_path = GetLibPath();
  }
  if (opts->default_imports == 0) {
    opts->default_imports = NewString(DEFAULT_IMPORTS);
  }

  opts->entry = argv[optind];
  return true;
}

void DestroyOpts(Opts *opts)
{
  if (opts->lib_path) free(opts->lib_path);
  if (opts->default_imports) free(opts->default_imports);
}
