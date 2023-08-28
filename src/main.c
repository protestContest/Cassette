#include "rec.h"
#include "args.h"
#include "lib.h"

int main(int argc, char *argv[])
{
  Args args = {NULL, ".", false, false};

  if (!ParseArgs(argc, argv, &args)) return 1;

  Heap mem;
  InitMem(&mem, 0);
  SetPrimitives(StdLib());

  Chunk *chunk;
  if (SniffFile(args.entry, ChunkTag())) {
    // read compiled chunk from file
    chunk = LoadChunk(args.entry);
  } else {
    // compile project into a chunk
    chunk = CompileChunk(&args, &mem);
  }

  if (!chunk) return 1;

  if (args.verbose) {
    Disassemble(chunk);
    Print("\n");
  }

  if (args.compile) {
    u8 *serialized = SerializeChunk(chunk);
    char *tape = ReplaceExtension(args.entry, "tape");
    if (WriteFile(tape, serialized, VecCount(serialized))) {
      return 0;
    } else {
      return 1;
    }
  } else {
    DestroyMem(&mem);
    InitMem(&mem, 0);

    VM vm;
    InitVM(&vm, &mem);
    vm.verbose = args.verbose;
    RunChunk(&vm, chunk);
    DestroyVM(&vm);
  }

  FreeChunk(chunk);
  DestroyMem(&mem);
}
