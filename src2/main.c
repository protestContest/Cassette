#include "compile.h"

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
  char *source = (char*)ReadFile(filename);

  Chunk chunk = Compile(source);
  Disassemble(&chunk);
  // HexDump("Chunk", chunk.data, VecCount(chunk.data));
  // HexDump("Constants", (u8*)chunk.constants, 4*VecCount(chunk.constants));
}
