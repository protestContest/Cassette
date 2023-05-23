#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "print.h"
#include "chunk.h"
#include "garbage.h"

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
    Val parsed = Parse(src, &compile_mem);
    if (!IsTagged(&compile_mem, parsed, "ok")) {
      Print("Error parsing \"");
      Print(filename);
      Print("\"\n");
      return;
    }

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
    Exit();
  }

  Source src = {filename, data, StrLen(data)};
  Val parsed = Parse(src, &mem);
  if (!IsTagged(&mem, parsed, "ok")) return;

  Val compiled = Compile(Tail(&mem, parsed), &mem);
  Chunk chunk = Assemble(compiled, &mem);

  u32 len = StrLen(filename);
  u32 ext = len;
  for (u32 i = 0; i < len; i++) {
    if (filename[len - i - 1] == '.') {
      ext = len - i;
      break;
    }
  }
  char compiled_name[ext + 5];
  for (u32 i = 0; i < ext; i++) {
    compiled_name[i] = filename[i];
  }
  compiled_name[ext] = 't';
  compiled_name[ext+1] = 'a';
  compiled_name[ext+2] = 'p';
  compiled_name[ext+3] = 'e';
  compiled_name[ext+4] = '\0';

  WriteChunk(&chunk, compiled_name);
}

void Usage(void)
{
  Print("Usage: cassette [-r] file\n");
}

int main(int argc, char *argv[])
{
  if (argc > 1) {
    if (StrEq(argv[1], "-h")) {
      Usage();
    } else if (StrEq(argv[1], "-r")) {
      if (argc < 3) {
        Usage();
      } else {
        CompileChunk(argv[2]);
      }
    } else {
      RunFile(argv[1]);
    }
  } else {
    Usage();
  }
}
