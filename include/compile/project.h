#pragma once
#include "compile/module.h"
#include "compile/opts.h"
#include "runtime/error.h"
#include "runtime/program.h"
#include "univ/hashmap.h"

/* A Project is a set of modules that can be built into a Program */

typedef struct {
  Module *modules; /* vec */
  HashMap mod_map;
  Program *program;
  u32 *build_list; /* vec */
  Opts *opts;
} Project;

Project *NewProject(Opts *opts);
void FreeProject(Project *project);
Error *AddProjectFile(Project *project, char *filename);
void ScanProjectFolder(Project *project, char *path);
Error *BuildProject(Project *project);
