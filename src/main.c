#include "project.h"
#include "univ/file.h"
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
/*
  program = project->program;
  program->trace = opts.debug;

  if (opts.debug) {
    u32 i;
    for (i = 0; i < VecCount(project->build_list); i++) {
      u32 modnum = project->build_list[i];
      Module *module = &project->modules[modnum];
      char *name = module->name ? SymbolName(module->name) : "*main*";
      fprintf(stderr, "%s\n", name);
      PrintNode(module->ast);
      fprintf(stderr, "\n");
    }
    DisassembleProgram(program);
    fprintf(stderr, "\n");
  }

  FreeProject(project);

  result = VMRun(program);
  if (!result.ok) {
    PrintError(result);
    PrintStackTrace(result);
    FreeStackTrace(result);
    FreeError(result.data.p);
  }

  FreeProgram(program);
*/
  return 0;
}
