#include <univ.h>

int main(int argc, char *argv[])
{
  char *source = ReadFile(argv[1]);
  printf("%s\n", source);
  return 0;
}
