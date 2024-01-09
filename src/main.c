/*
Welcome to the main file. You've chosen a good starting point. This is the main
project structure:

- base.h: Conveniences auto-included in every file (via the Makefile)
- cli.c: Handles option parsing and printing errors.
- debug.c: Handles debug output.
- main.c: The entry point. Depending on the CLI options, a project is compiled and run or saved.
- version.h: Holds the version number.
- app/: Abstracts all SDL-specific stuff, like canvas graphics and the main event loop.
- compile/: The parser & compiler, and associated files.
- device/: Runtime device drivers.
- mem/: Dynamic memory system.
- runtime/: The VM and associated files.
- univ/: General utility functions, and C stdlib abstractions

A typical invocation (`cassette file1.ct ...`) goes through this process:

- Options are parsed, which gives a list of files to include in the project
- The project is compiled into a chunk (see compile/project.c)
- If there are no errors, a VM is created, SDL initialized, and the SDL main loop is run
- Each time through the event loop, CanvasUpdate is called, which runs a bit of the chunk
- If the chunk is done and no windows were open, the main loop exits
*/

#include <stdio.h>
#ifndef GEN_SYMBOLS

#include "cli.h"
#include "debug.h"
#include "app/app.h"
#include "compile/project.h"
#include "runtime/chunk.h"
#include "runtime/vm.h"
#include "univ/math.h"
#include "univ/result.h"
#include "univ/str.h"
#include "univ/system.h"
#include "univ/font.h"

static Options opts;

static bool Update(void *arg);

int main(int argc, char *argv[])
{
  Chunk *chunk;
  Result result;
  VM vm;

  opts = ParseOpts(argc, argv);

  if (IsBinaryChunk(opts.filenames[0])) {
    result = ReadChunk(opts.filenames[0]);
  } else {
    result = BuildProject(opts);
  }

  if (!result.ok) {
    PrintError(result);
    Free(ResultError(result));
    return 1;
  }
  chunk = ResultItem(result);

  if (opts.debug) {
    Disassemble(chunk);
  }

  if (opts.compile) {
    char *filename = StrReplace(opts.filenames[0], ".ct", ".tape");
    WriteChunk(chunk, filename);
    return 0;
  }

  /* Ok, time to run the code */
  Seed(opts.seed);
  printf("Seed: %u\n", opts.seed);
  InitVM(&vm, chunk);
  if (opts.debug) {
    vm.trace = true;
    PrintTraceHeader(chunk->code.count);
  }
  InitApp();
  MainLoop(Update, &vm);

  DestroyVM(&vm);
  DestroyChunk(chunk);

  return 0;
}

/*
Since SDL has to run in a main loop, we use this function every tick to run some
of the VM code. When it returns false, the SDL main loop ends.
*/
static bool Update(void *arg)
{
  VM *vm = (VM*)arg;
  Result result = Run(1000, vm);

  if (!result.ok) {
    PrintRuntimeError(result, vm);
    Free(ResultError(result));
    return false;
  }

  return ResultValue(result) == True;
}

#endif
