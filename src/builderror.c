#include "builderror.h"

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
