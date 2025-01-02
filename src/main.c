#include "compile/opts.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/file.h"
#include "univ/str.h"

int main(int argc, char *argv[])
{
  Opts *opts;
  Error *error;
  Program *program = 0;

  opts = ParseOpts(argc, argv);
  if (!opts) return 1;

  if (!FileExists(opts->entry)) {
    fprintf(stderr, "Error: File \"%s\" not found\n", opts->entry);
    FreeOpts(opts);
    return 1;
  }

  if (StrEq(FileExt(opts->entry), opts->source_ext)) {
    Project *project = NewProject(opts);
    ScanProjectFolder(project, DirName(opts->entry));
    if (opts->lib_path) ScanProjectFolder(project, opts->lib_path);

    error = BuildProject(project);
    if (error) {
      PrintError(error);
      FreeError(error);
      FreeProject(project);
      FreeOpts(opts);
      return 1;
    }

    if (opts->compile) {
      char *path = ReplaceExt(opts->entry, ".tape");
      WriteProgramFile(project->program, path);
      FreeProject(project);
      FreeOpts(opts);
      free(path);
      return 0;
    }

    program = project->program;
    FreeProject(project);
  } else {
    program = ReadProgramFile(opts->entry);
    if (!program) {
      fprintf(stderr, "Error: Can't read program from \"%s\"\n", opts->entry);
      FreeOpts(opts);
      return 1;
    }
  }

  error = VMRun(program, opts->debug);
  if (error) {
    PrintError(error);
    FreeError(error);
  }

  FreeProgram(program);
  FreeOpts(opts);

  return 0;
}
