#include "rec.h"
#include "opts.h"
#include "univ/file.h"

int main(int argc, char *argv[])
{
  CassetteOpts opts = {argv[1], NULL, false, 0, 0};

  Chunk *chunk = LoadChunk(&opts);

  if (!chunk) return 1;

  if (opts.compile) {
    u8 *serialized = SerializeChunk(chunk);
    u32 serialized_size = SerializedSize(chunk);
    if (WriteFile(opts.entry, serialized, serialized_size)) {
      return 0;
    } else {
      return 1;
    }
  } else {
    VM vm;

    InitVM(&vm, &opts);
    RunChunk(&vm, chunk);
    DestroyVM(&vm);
  }

  FreeChunk(chunk);
}
