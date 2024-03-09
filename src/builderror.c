#include "builderror.h"
#include <univ.h>

BuildError *MakeBuildError(char *message, char *filename, u32 pos)
{
  BuildError *error = malloc(sizeof(BuildError));
  error->message = message;
  error->filename = CopyStr(filename, strlen(filename));
  error->pos = pos;
  return error;
}

void FreeBuildError(BuildError *error)
{
  free(error->filename);
  free(error);
}

void PrintError(BuildError *error)
{
  char *source = ReadFile(error->filename);
  u32 line = LineNum(source, error->pos);
  u32 col = ColNum(source, error->pos);
  printf("%s", ANSIRed);
  printf("%s:%d:%d: %s\n", error->filename, line, col, error->message);
  printf("%s", ANSINormal);
}
