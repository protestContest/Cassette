#pragma once

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  bool verbose;
} Args;

void PrintUsage(void);
bool ParseArgs(int argc, char *argv[], Args *args);
