#pragma once

typedef struct {
  char *entry;
  char *dir;
  bool compile;
  u32 verbose;
  u32 seed;
} CassetteOpts;
