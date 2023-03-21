#include "parse_gen.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("Usage: rye grammar.y\n");
    return 1;
  }

  Generate(argv[1]);
}
