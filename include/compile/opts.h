#pragma once

/*
These options control aspects of how a program is compiled and run.

`debug` controls whether the VM will trace its execution.
`lib_path` is a folder to scan for files to add to a project.
`entry` is the filename of the entry module.
`default_imports` is a list of modules to automatically import.
*/

typedef struct {
  bool debug;
  char *lib_path;
  char *entry;
  char *default_imports;
} Opts;

Opts *DefaultOpts(void);
Opts *ParseOpts(int argc, char *argv[]);
void FreeOpts(Opts *opts);
