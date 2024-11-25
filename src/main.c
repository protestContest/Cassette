#include "opts.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/file.h"

int main(int argc, char *argv[])
{
  Opts opts;
  Project *project;
  Error *error;

  if (!ParseOpts(argc, argv, &opts)) return 1;

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
    DestroyOpts(&opts);
    return 1;
  }

  project->program->trace = opts.debug;
  error = VMRun(project->program);
  if (error) {
    PrintError(error);
    FreeError(error);
  }

  FreeProgram(project->program);
  FreeProject(project);
  DestroyOpts(&opts);

  return 0;
}
