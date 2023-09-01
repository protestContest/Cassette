#include "rec.h"
#include "opts.h"
#include "lib.h"

#ifndef LIBCASSETTE

int main(int argc, char *argv[])
{
  CassetteOpts opts = DefaultOpts();

  if (!ParseArgs(argc, argv, &opts)) return 1;

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

    // for (u32 i = 0; i < 100; i++) {
      InitVM(&vm, &opts);
      RunChunk(&vm, chunk);
      DestroyVM(&vm);
    // }

    // PrintOpStats();
  }

  FreeChunk(chunk);
}

#endif
