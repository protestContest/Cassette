#include "compile.h"
#include "vm.h"
#include "run.h"
// #include "test.h"
// #include "fsevents.h"

static void OnChange(char *path, void *data)
{
  // VM *vm = (VM*)data;
  // RunFile(vm, path);
}

static void Usage(void)
{
  printf("Usage:\n");
  printf("  rye                     open a REPL\n");
  printf("  rye <image>             load an image and open a REPL\n");
  printf("  rye run <source>        run a script and exit\n");
  printf("  rye compile <source>    compile a script to an image\n");
  printf("  rye watch <source>      run a script and open a REPL, watching for file changes\n");
}

int main(int argc, char *argv[])
{
  VM vm;
  InitVM(&vm);

  if (argc == 1) {
    Image image;
    InitImage(&image);
    REPL(&vm, &image);
  } else if (argc == 2) {
    Image *image = ReadImage(argv[1]);
    RunImage(&vm, image);
    REPL(&vm, image);
  } else if (argc == 3) {
    if (strcmp(argv[1], "run") == 0) {
      Image image;
      InitImage(&image);
      Compile(argv[2], &image);
      RunImage(&vm, &image);
    } else if (strcmp(argv[1], "compile") == 0) {
      Image image;
      InitImage(&image);
      CompileModules(argv[2], &image);
      WriteImage(&image, "rye.image");
    } else if (strcmp(argv[1], "watch") == 0) {
      // WatchFile(argv[1], OnChange, &vm);
      printf("Not implemented\n");
    } else {
      Usage();
    }
  } else {
    Usage();
  }
}
