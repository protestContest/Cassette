#pragma once

typedef struct {
  char *message;
  char *filename;
  u32 pos;
} BuildError;

BuildError *MakeBuildError(char *message, char *filename, u32 pos);
void FreeBuildError(BuildError *error);
