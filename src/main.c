#include "project.h"
#include "univ/file.h"
#include "vm.h"
#include <unistd.h>

#define VERSION "2.0.0-alpha"

typedef struct {
  bool debug;
  char *lib_path;
  char *entry;
} Opts;

static void Usage(void)
{
  fprintf(stderr, "Usage: cassette [opts] script\n");
  fprintf(stderr, "  -v            Print version\n");
  fprintf(stderr, "  -d            Enable debug mode\n");
  fprintf(stderr, "  -L lib_path   Library search path (defaults to $CASSETTE_PATH)\n");
}

static Opts ParseOpts(int argc, char *argv[])
{
  Opts opts;
  int ch;
  opts.debug = false;
  opts.lib_path = getenv("CASSETTE_PATH");
  opts.entry = 0;

  while ((ch = getopt(argc, argv, "vdL:")) >= 0) {
    switch (ch) {
    case 'd':
      opts.debug = true;
      break;
    case 'L':
      opts.lib_path = optarg;
      break;
    case 'v':
      printf("Cassette %s\n", VERSION);
      exit(0);
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
  Result result;

  project = NewProject();
  AddProjectFile(project, opts.entry);
  ScanProjectFolder(project, DirName(opts.entry));
  ScanProjectFolder(project, opts.lib_path);

  error = BuildProject(project);
  if (error) {
    PrintError(error);
    FreeError(error);
    FreeProject(project);
    return 1;
  }

  project->program->trace = opts.debug;;
  result = VMRun(project->program);
  if (!result.ok) {
    PrintError(result.data.p);
    PrintStackTrace(result);
    FreeStackTrace(result);
    FreeError(result.data.p);
  }

  return 0;
}
