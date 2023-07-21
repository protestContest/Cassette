#pragma once

typedef struct {
  bool compile;
  bool version;
  bool help;
  bool trace;
  bool print_ast;
  char *filename;
  char *module_path;
} Opts;

Opts ParseArgs(u32 argc, char *argv[]);
