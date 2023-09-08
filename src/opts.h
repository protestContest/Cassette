#pragma once

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  u32 verbose;
  u32 seed;
} CassetteOpts;

#ifndef LIBCASSETTE
void PrintUsage(void);
CassetteOpts DefaultOpts(void);
bool ParseArgs(int argc, char *argv[], CassetteOpts *args);
#endif
