#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "print.h"
#include "chunk.h"
#include "garbage.h"

static void ReplaceExtension(char *filename, char *dst);

void RunFile(char *filename)
{
  Chunk chunk;

  if (SniffFile(filename, ChunkSignature)) {
    if (!ReadChunk(filename, &chunk)) {
      Print("Error reading \"");
      Print(filename);
      Print("\"\n");
      return;
    }
  } else {
    char *data = (char*)ReadFile(filename);
    if (!data) {
      Print("Could not open file \"");
      Print(filename);
      Print("\"\n");
      return;
    }

    Mem compile_mem;
    InitMem(&compile_mem, 0);

    Source src = {filename, data, StrLen(data)};
    PrintSource(src);

    Val parsed = Parse(src, &compile_mem);
    if (!IsTagged(&compile_mem, parsed, "ok")) {
      Print("Error parsing \"");
      Print(filename);
      Print("\"\n");
      return;
    }

    // return;

    Val compiled = Compile(Tail(&compile_mem, parsed), &compile_mem);
    if (IsNil(compiled)) return;

    chunk = Assemble(compiled, &compile_mem);
    DestroyMem(&compile_mem);
  }

  Mem mem;
  InitMem(&mem, 0);
  mem.symbols = chunk.symbols;

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);

  PrintMem(&mem);
}

void CompileChunk(char *filename)
{
  Mem mem;
  InitMem(&mem, 0);

  char *data = (char*)ReadFile(filename);
  if (!data) {
    Print("Could not open file");
    return;
  }

  Source src = {filename, data, StrLen(data)};
  Val parsed = Parse(src, &mem);
  if (!IsTagged(&mem, parsed, "ok")) {
    Print("Error parsing \"");
    Print(filename);
    Print("\"\n");
    return;
  }

  // Val compiled = Compile(Tail(&mem, parsed), &mem);
  // if (IsNil(compiled)) return;
  // Chunk chunk = Assemble(compiled, &mem);

  // DestroyMem(&mem);

  // char outfile[StrLen(filename) + 5];
  // ReplaceExtension(filename, outfile);

  // WriteChunk(&chunk, outfile);
}

void Usage(void)
{
  Print("Usage: cassette [-c] file\n");
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    Usage();
    return 0;
  }

  if (StrEq(argv[1], "-h")) {
    Usage();
    return 0;
  }

  if (StrEq(argv[1], "-c")) {
    if (argc < 3) {
      Usage();
      return 0;
    }

    CompileChunk(argv[2]);
    return 0;
  }

  RunFile(argv[1]);
  return 0;
}

static void ReplaceExtension(char *filename, char *dst)
{
  u32 len = StrLen(filename);
  u32 ext = len;
  for (u32 i = 0; i < len; i++) {
    if (filename[len - i - 1] == '.') {
      ext = len - i;
      break;
    }
  }

  for (u32 i = 0; i < ext; i++) {
    dst[i] = filename[i];
  }
  dst[ext] = 't';
  dst[ext+1] = 'a';
  dst[ext+2] = 'p';
  dst[ext+3] = 'e';
  dst[ext+4] = '\0';
}
