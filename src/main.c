#include "result.h"
#include "univ/str.h"
#include "runtime/chunk.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "cli.h"
#include "canvas/canvas.h"
#include <stdio.h>

#ifdef DEBUG
#include "debug.h"
#endif

static Options opts;

#ifdef CANVAS
bool CanvasUpdate(void *arg)
{
  VM *vm = (VM*)arg;
  Result result = Run(vm, 1000);

  if (!result.ok) {
    PrintError(result);
    return false;
  }

  return result.value == True || vm->canvases.count > 0;
}
#endif

int main(int argc, char *argv[])
{
  Chunk chunk;
  Result result;
  VM vm;

  if (argc < 2) return Usage();
  opts = ParseOpts(argc, argv);

  InitChunk(&chunk);

  if (opts.project) {
    if (argc < 3) return Usage();
    result = BuildProject(argv[opts.file_args], &chunk);
  } else {
    result = BuildScripts(argc - opts.file_args, argv + opts.file_args, &chunk);
  }

  if (!result.ok) {
    PrintError(result);
    DestroyChunk(&chunk);
    return 1;
  }

#ifdef DEBUG
  Disassemble(&chunk);
#endif

  InitVM(&vm, &chunk);

#ifdef CANVAS
  InitGraphics();
  MainLoop(CanvasUpdate, &vm);
#else
  result = OkResult(True);
  while (result.ok && result.value == True) {
    result = Run(&vm, 1);
    if (opts.step && WaitForInput()) return 0;
  }

  if (!result.ok) {
    PrintError(result);
  }
#endif

  DestroyVM(&vm);
  DestroyChunk(&chunk);

  return 0;
}
