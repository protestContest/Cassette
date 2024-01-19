/*
Welcome to the main file. You've chosen a good starting point.

This file, depending on the CLI options, loads or compiles a project and saves
the result or runs it in a VM. The VM is run in the context of an SDL main loop
to allow for graphics.

The other top-level files are utility files. This is the project structure:

- base: Auto-included in every file; mainly defines convenient types.
- cli: Handles option parsing and printing errors.
- debug: Handles debug output.
- gen_symbols: An alternative main function that just calculates and prints
  symbol values.
- version: Version numbers.

The rest of the code is organized into these folders:
- app/:
    - app: Top-level SDL app stuff, like initialization and the main event loop.
    - canvas: Functions to draw into an SDL window.
- compile/:
    - lex: Functions to parse a file into a token stream.
    - parse: Functions to parse a file into an AST.
    - compile: Functions to compile a module's AST into a chunk.
    - project: Functions to compile files and link them into a chunk.
    - env: Functions for the compiler's environment.
- runtime/:
    - chunk: Functions for a code chunk object.
    - vm: Functions to run instructions from a chunk.
    - ops: Opcode definitions.
    - primitives: Primitive function definitions.
    - env: Functions for the VM's environment.
    - stacktrace: Functions for generating a stack trace.
    - mem/: The dynamic memory system, including functions for manipulating
      value objects and garbage collection.
    - device/: The IO device system, including drivers for the console,
      filesystem, serial ports, system info, and windows.
- univ/: General-purpose utility functions, often wrapping libc functions (for
  portability to systems without libc).
    - file: File IO functions.
    - font: Functions to list installed fonts.
    - hash: Functions to generate hashes and CRCs.
    - hashmap: A basic hash map.
    - math: General math functions.
    - result: An optional type, wrapping a value, pointer, or error object.
    - serial: Functions for serial ports.
    - str: String functions.
    - system: Memory allocation and other general system functions.
    - vec: Dynamic array implementations.
*/

#ifndef GEN_SYMBOLS

#include "cli.h"
#include "debug.h"
#include "app/app.h"
#include "app/canvas.h"
#include "compile/project.h"
#include "runtime/chunk.h"
#include "runtime/vm.h"
#include "univ/math.h"
#include "univ/result.h"
#include "univ/str.h"
#include "univ/system.h"
#include "univ/font.h"
#include <stdio.h>

static bool Update(void *arg);

int main(int argc, char *argv[])
{
  Chunk *chunk;
  Result result;
  VM vm;

  Options opts = ParseOpts(argc, argv);

  /* Read or build a chunk */
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
  if (opts.debug) Disassemble(chunk);

  if (opts.compile) {
    /* Write the chunk to a file */
    char *filename = StrReplace(opts.filenames[0], ".ct", ".tape");
    WriteChunk(chunk, filename);
    return 0;
  }

  /* Time to run the code */
  Seed(opts.seed);
  printf("Seed: %u\n", opts.seed);

  InitVM(&vm, chunk);
  if (opts.debug) {
    vm.trace = true;
    PrintTraceHeader(chunk->code.count);
  }

  /* Instructions will be run in Update, from the event loop */
  InitApp();
  MainLoop(Update, &vm);

  DestroyVM(&vm);
  DestroyChunk(chunk);

  return 0;
}

/* Since SDL has to run in a main loop, we use this function every tick to run some
of the VM code. When it returns false, the SDL main loop ends. */
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
