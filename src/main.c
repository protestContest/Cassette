#include "vm.h"

static void REPL(VM *vm)
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

static char *ReadFile(const char *path)
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

static void RunFile(VM *vm, const char *path)
{
  char *src = ReadFile(path);
  Status result = Interpret(vm, src);
  free(src);

  if (result == Error) exit(1);
}

int main(int argc, char *argv[])
{
  VM *vm = NewVM();

  if (argc == 1) {
    REPL(vm);
  } else if (argc == 2) {
    RunFile(vm, argv[1]);
  } else {
    Fatal("Usage: rye [path]\n");
  }

  FreeVM(vm);
}
