#include "result.h"
#include "source.h"
#include "univ/string.h"
#include "univ/system.h"
#include <stdio.h>

BuildResult BuildOk(Val value)
{
  BuildResult result;
  result.ok = true;
  result.value = value;
  result.error = 0;
  result.filename = 0;
  result.source = 0;
  result.pos = 0;
  return result;
}

BuildResult BuildError(char *error, char *filename, char *source, u32 pos)
{
  BuildResult result;
  result.ok = false;
  result.value = Error;
  result.error = error;
  result.filename = filename;
  result.source = source;
  result.pos = pos;
  return result;
}

void PrintBuildError(BuildResult error)
{
  u32 line, col;

  if (error.filename && !error.source) {
    error.source = ReadFile(error.filename);
    if (error.source) {
      line = LineNum(error.source, error.pos);
      col = ColNum(error.source, error.pos);
    }
  }

  if (error.filename) {
    printf("%s", error.filename);
    if (error.source) {
      printf(":%d:%d", line+1, col+1);
    }
    printf(" ");
  }

  printf("Error: %s\n", error.error);
  if (error.source) {
    PrintSourceContext(error.pos, error.source, 3);
  }
}
