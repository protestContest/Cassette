#include "rec.h"
#include "opts.h"
#include "lib.h"

#ifndef LIBCASSETTE

int main(int argc, char *argv[])
{
  CassetteOpts opts = DefaultOpts();
  if (!ParseArgs(argc, argv, &opts)) return 1;

#ifdef WITH_CANVAS
  if (!InitCanvas(400, 240)) {
    Print("SDL Error\n");
    return 1;
  }
#endif

  Chunk *chunk = LoadChunk(&opts);

  if (!chunk) return 1;

  if (opts.verbose > 1) {
    Disassemble(chunk);
    Print("\n");
  }

  if (opts.compile) {
    u8 *serialized = SerializeChunk(chunk);
    char *tape = ReplaceExtension(opts.entry, "tape");
    if (WriteFile(tape, serialized, VecCount(serialized))) {
      return 0;
    } else {
      return 1;
    }
  } else {
    VM vm;

    InitVM(&vm, &opts);
    RunChunk(&vm, chunk);
    DestroyVM(&vm);

#ifdef WITH_CANVAS
    if (ShouldShowCanvas()) {
      ShowCanvas();
    }
#endif
  }

  FreeChunk(chunk);
#ifdef WITH_CANVAS
  DestroyCanvas();
#endif
}

#endif
