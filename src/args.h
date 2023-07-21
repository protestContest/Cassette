#pragma once

typedef struct {
  bool compile;
  bool version;
  bool help;
  bool trace;
  char *filename;
} Opts;

Opts ParseArgs(u32 argc, char *argv[]);
