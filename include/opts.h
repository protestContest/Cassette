#pragma once

typedef struct {
  bool debug;
  char *lib_path;
  char *entry;
  char *default_imports;
} Opts;

bool ParseOpts(int argc, char *argv[], Opts *opts);
void DestroyOpts(Opts *opts);
