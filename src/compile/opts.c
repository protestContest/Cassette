#include "compile/opts.h"
#include "univ/file.h"
#include "univ/str.h"
#include "univ/vec.h"
#include <unistd.h>

#define DEFAULT_EXT ".ct"

static void Usage(void)
{
  fprintf(stderr, "Usage: cassette [opts] script\n");
  fprintf(stderr, "  -v            Print version\n");
  fprintf(stderr, "  -c            Compile project\n");
  fprintf(stderr, "  -d            Enable debug mode\n");
  fprintf(stderr, "  -L lib_path   Library search path (default $CASSETTE_PATH)\n");
  fprintf(stderr, "  -m manifest   Project file list (default all .ct files in current directory)\n");
}

/* Search for an existing library path in this order:
 * - Env variable $CASSETTE_PATH
 * - $HOME/.local/share/cassette
 * - /usr/local/share/cassette
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
  printf("Cassette %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
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
  opts->manifest = 0;
  opts->source_ext = NewString(DEFAULT_EXT);
  opts->program_args = 0;
  return opts;
}

Opts *ParseOpts(int argc, char *argv[])
{
  Opts *opts = DefaultOpts();
  int ch, i;

  while ((ch = getopt(argc, argv, "chvdL:m:")) >= 0) {
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
    case 'm':
      opts->manifest = NewString(optarg);
      break;
    default:
      Usage();
      FreeOpts(opts);
      return 0;
    }
  }

  if (!opts->manifest) {
    if (optind == argc) {
      Usage();
      FreeOpts(opts);
      exit(1);
    }

    opts->entry = argv[optind++];
  }

  for (i = optind; i < argc; i++) {
    VecPush(opts->program_args, NewString(argv[i]));
  }
  return opts;
}

void FreeOpts(Opts *opts)
{
  if (opts->lib_path) free(opts->lib_path);
  if (opts->manifest) free(opts->manifest);
  if (opts->source_ext) free(opts->source_ext);
  if (opts->program_args) {
    u32 i;
    for (i = 0; i < VecCount(opts->program_args); i++) {
      free(opts->program_args[i]);
    }
    FreeVec(opts->program_args);
  }
  free(opts);
}
