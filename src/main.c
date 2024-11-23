#include "project.h"
#include "univ/file.h"
#include "univ/str.h"
#include "vm.h"
#include <unistd.h>

#define VERSION "2.0.0"

#define DEFAULT_IMPORTS "Record, IO, List, Enum, Map, Math, String, VM (panic!), Value (typeof, symbol_name, nil?, integer?, symbol?, pair?, tuple?, binary?, error?, format, inspect, hash)"

typedef struct {
  bool debug;
  char *lib_path;
  char *entry;
  char *default_imports;
} Opts;

static void Usage(void)
{
  fprintf(stderr, "Usage: cassette [opts] script\n");
  fprintf(stderr, "  -v            Print version\n");
  fprintf(stderr, "  -d            Enable debug mode\n");
  fprintf(stderr, "  -L lib_path   Library search path (defaults to $CASSETTE_PATH)\n");
  fprintf(stderr, "  -i imports    List of modules to auto-import\n");
}

static char *GetLibPath(void)
{
  char *path = getenv("CASSETTE_PATH");
  if (path && DirExists(path)) return path;
  path = StrCat(HomeDir(), "/.local/share/cassette");
  if (DirExists(path)) return path;
  path = "/usr/local/share/cassette";
  if (DirExists(path)) return path;
  return 0;
}

static Opts ParseOpts(int argc, char *argv[])
{
  Opts opts;
  int ch;
  opts.debug = false;
  opts.lib_path = GetLibPath();
  opts.entry = 0;
  opts.default_imports = DEFAULT_IMPORTS;

  while ((ch = getopt(argc, argv, "i:vdL:")) >= 0) {
    switch (ch) {
    case 'd':
      opts.debug = true;
      break;
    case 'L':
      opts.lib_path = optarg;
      break;
    case 'v':
      printf("Cassette %s\n", VERSION);
      printf("Library path: %s\n", opts.lib_path);
      exit(0);
    case 'i':
      opts.default_imports = optarg;
      break;
    default:
      Usage();
      exit(1);
    }
  }

  if (optind == argc) {
    Usage();
    exit(1);
  }

  opts.entry = argv[optind];
  return opts;
}

int main(int argc, char *argv[])
{
  Opts opts = ParseOpts(argc, argv);
  Project *project;
  Error *error;

  project = NewProject();
  project->default_imports = opts.default_imports;
  AddProjectFile(project, opts.entry);
  ScanProjectFolder(project, DirName(opts.entry));
  if (opts.lib_path) ScanProjectFolder(project, opts.lib_path);

  error = BuildProject(project);
  if (error) {
    PrintError(error);
    FreeError(error);
    FreeProject(project);
    return 1;
  }

  project->program->trace = opts.debug;
  error = VMRun(project->program);
  if (error) {
    PrintError(error);
    PrintStackTrace(error->data);
    FreeStackTrace(error->data);
    FreeError(error);
  }

  FreeProgram(project->program);
  FreeProject(project);

  return 0;
}
