#include "compile/parse.h"
#include <univ/io.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
  char *src = "(4 + 1)";
  printf("\n%s\n\n", src);

  if (Parse(src)) {
    printf("Ok\n");
  } else {
    printf("Error\n");
  }
}
