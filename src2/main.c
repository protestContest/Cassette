#include "compile.h"
#include "vm.h"
#include "module.h"

void Usage(void)
{
  Print("Usage:\n");
  Print("  cassette script.csst\n");
}

i32 main(i32 argc, char **argv)
{
  if (argc < 2) {
    Usage();
    return 0;
  }

  char *filename = argv[1];

  Mem mem;
  InitMem(&mem);
  Val ast = LoadModules(filename, &mem);
  PrintVal(ast, &mem);
  Print("\n");
  Seq code = Compile(ast, &mem);
  PrintSeq(code, &mem);
  Chunk chunk = Assemble(code, &mem);
  DestroyMem(&mem);

  VM vm;
  InitVM(&vm);
  RunChunk(&vm, &chunk);
  // HexDump("Chunk", chunk.data, VecCount(chunk.data));
}
