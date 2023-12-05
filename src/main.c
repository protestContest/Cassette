/*
Welcome to the main file. You've chosen a good starting point. I'll outline the
major project structure here.

Files in the top-level folder, "src", are generally user interface related
files. ("base.h" is an exception, it defines some C conveniences.) All debug
output is kept in "debug.c", which can be switched off at compile time when the
DEBUG flag is off (check the Makefile). "result.c" is a wrapper for errors, and
used by the compiler and VM for intermediate results. "cli.c" handles parsing
CLI options, printing errors to the console, etc.

The "univ" folder contains general utility code, often simply wrapping the C
standard library. This should make porting to systems without libc easier, since
only this folder would need to be ported. It also contains conveniences like a
hashmap, dynamic vector, rolling hash, math, and string stuff.

The "mem" folder contains the dynamic memory system and symbol table
implementation. Basically everything works with a memory heap defined here. The
"mem.h" file in particular defines the representation of values in the system.

The "compile" folder contains implementations of the lexer, parser, compiler,
and linker ("project.c").

The "runtime" folder contains the VM (and its supporting files) and defines
the "Chunk" type, which is an executable object created by the compiler and run
on the VM.

The "canvas" folder contains the implementation of the canvas and any other SDL2
dependent code, and can be switched off with the CANVAS flag.
*/

#include "result.h"
#include "univ/str.h"
#include "runtime/chunk.h"
#include "compile/project.h"
#include "runtime/vm.h"
#include "cli.h"
#include "canvas/canvas.h"
#include <stdio.h>
#include <unistd.h>
#include "debug.h"
#include "univ/serial.h"

static Options opts;

static bool CanvasUpdate(void *arg);

int main(int argc, char *argv[])
{
  Chunk chunk;
  Result result;
  VM vm;

  if (argc < 2) return Usage();
  opts = ParseOpts(argc, argv);

  InitChunk(&chunk);

  /* We either build from a manifest of source files, or the source files passed
  in directly. */
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

  /* Ok, time to run the code */
  InitVM(&vm, &chunk);
  if (opts.trace) vm.trace = true;
  InitGraphics();
  MainLoop(CanvasUpdate, &vm);

  /* Not sure why we bother cleaning up if we're just about to exit, but somehow
  it feels right */
  DestroyVM(&vm);
  DestroyChunk(&chunk);

  return 0;
}

/*
Since SDL has to run in a main loop, we use this function every tick to run some
of the VM code. When it returns false, the SDL main loop ends. It returns false
once it's done running code, but only if there are no windows open.
*/
static bool CanvasUpdate(void *arg)
{
  VM *vm = (VM*)arg;
  Result result = Run(vm, 1000);

  if (!result.ok) {
    PrintError(result);
    return false;
  }

  return result.value == True || AnyWindowsOpen(vm);
}
