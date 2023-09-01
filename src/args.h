#pragma once

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  u32 verbose;
} Args;

#ifndef LIBCASSETTE
void PrintUsage(void);
Args DefaultArgs(void);
bool ParseArgs(int argc, char *argv[], Args *args);
#endif
