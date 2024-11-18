#pragma once

typedef struct {
  char **message;
  char **filename;
  u32 pos;
  u32 length;
  void *data;
} Error;

Error **NewError(char *message, char *filename, u32 pos, u32 length);
void FreeError(Error **error);
void PrintError(Error **error);
void PrintSourceLine(char *text, u32 pos);
