#include "rec.h"

static void PrintUsage(void)
{
  Print("Usage: cassette script.cst [options]\n");
  Print("\n");
  Print("  -h            Prints this message and exits\n");
  Print("  -p sources    Project directory contiaining scripts\n");
}

static bool ParseArgs(int argc, char *argv[], char **entry, char **source_dir)
{
  if (argc < 2) {
    PrintUsage();
    return false;
  }

  for (i32 i = 1; i < argc; i++) {
    if (StrEq(argv[i], "-h")) {
      PrintUsage();
      return false;
    } else if (StrEq(argv[i], "-p") && i < argc-1) {
      *source_dir = argv[i+1];
      i++;
    } else {
      *entry = argv[i];
    }
  }

  return entry != NULL;
}

int main(int argc, char *argv[])
{
  char *source_dir = ".";
  char *entry = NULL;

  if (!ParseArgs(argc, argv, &entry, &source_dir)) return 1;

  Heap mem;
  InitMem(&mem, 0);

  ModuleResult result = LoadProject(source_dir, entry, &mem);
  if (!result.ok) {
    Print(result.error.message);
    Print(": ");
    Inspect(result.error.expr, &mem);
    Print("\n");
    return 1;
  }

  Chunk chunk;
  InitChunk(&chunk);
  Assemble(result.module.code, &chunk, &mem);

  Disassemble(&chunk);
  Print("\n");

  DestroyMem(&mem);
  InitMem(&mem, 0);

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);

  DestroyChunk(&chunk);
  DestroyVM(&vm);
  DestroyMem(&mem);
}
