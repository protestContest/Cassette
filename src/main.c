#include "parse.h"
#include <univ.h>

int main(int argc, char *argv[])
{
  char *source = ReadFile(argv[1]);
  i32 length, result;
  printf("%s\n", source);
  length = strlen(source);
  result = Parse(source, length);

  if (IsError(result)) {
    PrintError(result, source);
  } else {
    PrintAST(result);
  }
}
