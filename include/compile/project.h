#pragma once
#include "compile/module.h"
#include "compile/opts.h"
#include "runtime/error.h"
#include "runtime/program.h"
#include "univ/hashmap.h"

/*
 * A Project keeps track of the construction of a Program from source files. A program is built in
 * these steps:
 *
 * 1. All project files are parsed up to their module name and imports to generate a module map.
 * 2. Starting with the entry file, a build list is constructed of only the imported modules, in
 *    order of dependency.
 * 3. Each module in the build list is parsed and compiled.
 * 4. The compiled modules are linked into a Program in order of dependency. Each module is executed
 *    before the modules that import it. The entry module is executed last.
 */

typedef struct {
  Module *modules; /* vec */
  HashMap mod_map;
  Program *program;
  u32 *build_list; /* vec */
  Opts *opts;
  u32 entry_index;
} Project;

Project *NewProject(Opts *opts);
void FreeProject(Project *project);
Error *AddProjectFile(Project *project, char *filename);
void ScanProjectFolder(Project *project, char *path);
Error *ScanManifest(Project *project, char *path);
Error *BuildProject(Project *project);

#if PROFILE
#include "runtime/stats.h"
extern StatGroup *build_stats;
#endif
