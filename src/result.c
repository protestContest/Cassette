#include "result.h"
#include <univ.h>

Result Ok(void *value)
{
  Result result = {true, 0};
  result.value = value;
  return result;
}

Result Error(char *message, char *filename, u32 pos)
{
  Result result = {false, 0};
  ErrorDetail *error = malloc(sizeof(ErrorDetail));
  error->message = message;
  error->filename = CopyStr(filename, strlen(filename));
  error->pos = pos;
  return result;
}

void FreeResult(Result result)
{
  if (result.value) free(result.value);
}

void PrintError(Result result)
{
  ErrorDetail *error = result.value;
  char *source = ReadFile(error->filename);
  u32 line = LineNum(source, error->pos);
  u32 col = ColNum(source, error->pos);
  printf("%s", ANSIRed);
  printf("%s:%d:%d: %s\n", error->filename, line, col, error->message);
  printf("%s", ANSINormal);
}
