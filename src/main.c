#include "compile/opts.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/file.h"

int main(int argc, char *argv[])
{
  Opts *opts;
  Project *project;
  Error *error;

  opts = ParseOpts(argc, argv);
  if (!opts) return 1;

  project = NewProject(opts);
  ScanProjectFolder(project, DirName(opts->entry));
  if (opts->lib_path) ScanProjectFolder(project, opts->lib_path);

  error = BuildProject(project);
  if (error) {
    PrintError(error);
    FreeError(error);
    FreeProject(project);
    return 1;
  }

  error = VMRun(project->program, opts->debug);
  if (error) {
    PrintError(error);
    FreeError(error);
  }

  FreeProgram(project->program);
  FreeProject(project);

  return 0;
}
