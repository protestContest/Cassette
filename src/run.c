#include "run.h"

void REPL(VM *vm)
{
  char line[1024];
  while (true) {
    printf("? ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    Interpret(vm, line);
  }
}

char *ReadFile(const char *path)
{
  FILE *file = fopen(path, "rb");
  fseek(file, 0L, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  char *buffer = (char*)malloc(size + 1);
  size_t num_read = fread(buffer, sizeof(char), size, file);
  buffer[num_read] = '\0';
  fclose(file);
  return buffer;
}

void RunFile(VM *vm, const char *path)
{
  char *src = ReadFile(path);
  Interpret(vm, src);
  free(src);
}