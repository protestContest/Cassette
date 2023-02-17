#include "console.h"

int main(int argc, char *argv[])
{
  Print("Hello, world!\n");
}


// #include "compile/compile.h"
// #include "runtime/vm.h"
// #include "platform/console.h"

// // static void OnChange(char *path, void *data)
// // {
// //   // VM *vm = (VM*)data;
// //   // RunFile(vm, path);
// // }

// static void Usage(void)
// {
//   Print("Usage:\n");
//   Print("  rye                     open a REPL\n");
//   Print("  rye <image>             load an image and open a REPL\n");
//   Print("  rye run <source>        run a script and exit\n");
//   Print("  rye compile <source>    compile a script to an image\n");
//   Print("  rye watch <source>      run a script and open a REPL, watching for file changes\n");
// }

// int main(int argc, char *argv[])
// {
//   VM vm;
//   InitVM(&vm);

//   // if (argc == 1) {
//   //   Image image;
//   //   InitImage(&image);
//   //   REPL(&vm, &image);
//   // } else
//   if (argc == 2) {
//     Image *image = ReadImage(argv[1]);
//     RunImage(&vm, image);
//     // REPL(&vm, image);
//   // } else if (argc == 3) {
//   //   if (strcmp(argv[1], "run") == 0) {
//   //     Image image;
//   //     InitImage(&image);
//   //     Compile(argv[2], &image);
//   //     RunImage(&vm, &image);
//   //   } else if (strcmp(argv[1], "compile") == 0) {
//   //     Image image;
//   //     InitImage(&image);
//   //     CompileModules(argv[2], &image);
//   //     WriteImage(&image, "rye.image");
//   //   } else if (strcmp(argv[1], "watch") == 0) {
//   //     // WatchFile(argv[1], OnChange, &vm);
//   //     Print("Not implemented\n");
//   //   } else {
//   //     Usage();
//   //   }
//   } else {
//     Usage();
//   }
// }
