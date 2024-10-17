#pragma once
#include "error.h"
#include "module.h"
#include "program.h"
#include "univ/hashmap.h"

/* A Project is a set of modules that can be built into a Program */

typedef struct {
  Module *modules;
  HashMap mod_map;
  Program *program;
  u32 *build_list;
} Project;

Project *NewProject(void);
void FreeProject(Project *project);
Error *AddProjectFile(Project *project, char *filename);
void ScanProjectFolder(Project *project, char *path);
Error *BuildProject(Project *project);
