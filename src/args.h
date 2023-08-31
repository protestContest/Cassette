#pragma once

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  u32 verbose;
} Args;

void PrintUsage(void);
bool ParseArgs(int argc, char *argv[], Args *args);
