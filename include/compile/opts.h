#pragma once

typedef struct {
  bool debug;
  char *lib_path;
  char *entry;
  char *default_imports;
} Opts;

Opts *DefaultOpts(void);
Opts *ParseOpts(int argc, char *argv[]);
void FreeOpts(Opts *opts);
