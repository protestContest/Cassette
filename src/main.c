#include "compile/opts.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "univ/file.h"
#include "univ/str.h"

/* This file is the command-line interface to Cassette. It's job is to build and/or run Cassette
 * programs based on the given options. There are three * basic modes:
 *
 * 1. Compile and run a project from source files (default).
 * 2. Compile a project to an image to run later (-c option).
 * 3. Run a previously-compiled image.
 *
 * The entry file must always be specified. This is either the entry source file for a program or a
 * compiled image file. See "opts.h" for a description of all options.
 */

int main(int argc, char *argv[])
{
  Opts *opts;
  Error *error = 0;
  Program *program = 0;

  opts = ParseOpts(argc, argv);
  if (!opts) return 1;

  if (opts->manifest || StrEq(FileExt(opts->entry), opts->source_ext)) {
    Project *project = NewProject(opts);
    if (opts->manifest) {
      error = ScanManifest(project, opts->manifest);
    } else {
      error = AddProjectFile(project, project->opts->entry);
      if (!error) ScanProjectFolder(project, DirName(opts->entry));
    }
    if (!error && opts->lib_path) ScanProjectFolder(project, opts->lib_path);

    if (!error) error = BuildProject(project);

    if (error) {
      PrintError(error);
      FreeError(error);
      FreeProject(project);
      FreeOpts(opts);
      return 1;
    }

    if (opts->compile) {
      char *path = opts->entry ? opts->entry : opts->manifest;
      path = ReplaceExt(path, ".tape");
      WriteProgramFile(project->program, path);
      FreeProject(project);
      FreeOpts(opts);
      free(path);
      return 0;
    }

    program = project->program;
    FreeProject(project);
  } else {
    Error *error = ReadProgramFile(opts->entry, &program);
    if (error) {
      PrintError(error);
      FreeOpts(opts);
      return 1;
    }
  }

  error = VMRun(program, opts);
  if (error) {
    PrintError(error);
    FreeError(error);
  }

  FreeProgram(program);
  FreeOpts(opts);

  return 0;
}
