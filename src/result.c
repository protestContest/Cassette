#include "result.h"
#include "compile/source.h"
#include "univ/string.h"
#include "univ/system.h"
#include <stdio.h>

Result OkResult(Val value)
{
  Result result;
  result.ok = true;
  result.value = value;
  result.error = 0;
  result.filename = 0;
  result.pos = 0;
  return result;
}

Result ErrorResult(char *error, char *filename, u32 pos)
{
  Result result;
  result.ok = false;
  result.value = Error;
  result.error = error;
  result.filename = filename;
  result.pos = pos;
  return result;
}

void PrintError(Result error)
{
  u32 line, col;
  char *source = 0;

  if (error.filename) {
    source = ReadFile(error.filename);
    if (source) {
      line = LineNum(source, error.pos);
      col = ColNum(source, error.pos);
    }
  }

  printf("%s", ANSIRed);
  if (error.filename) {
    printf("%s", error.filename);
    if (source) {
      printf(":%d:%d", line+1, col+1);
    }
    printf(" ");
  }

  printf("Error: %s\n", error.error);
  if (source) {
    PrintSourceContext(error.pos, source, 3);
  }
  printf("%s", ANSINormal);
}
