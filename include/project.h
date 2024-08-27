#pragma once
#include "module.h"
#include "result.h"
#include "univ/hashmap.h"

/* A Project is a set of modules that can be built into a Program */

typedef struct {
  Module *modules;
  u32 *build_list;
  HashMap mod_map;
} Project;

Project *NewProject(char *entryfile, char *searchpath);
void FreeProject(Project *project);
Result BuildProject(Project *project);
