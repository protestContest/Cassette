#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "print.h"
#include "chunk.h"

#define DEBUG DEBUG_LEXER | DEBUG_PARSE | DEBUG_PARSE2 | DEBUG_COMPILE | DEBUG_ASSEMBLE | DEBUG_VM

void RunFile(char *filename)
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

#if DEBUG
  PrintTree(Tail(&mem, parsed), &mem);
  Print("\n");
#endif

  Val compiled = Compile(Tail(&mem, parsed), &mem);
  Chunk chunk = Assemble(compiled, &mem);

// // #if DEBUG
  Print("──╴Disassembled╶──\n");
  Disassemble(&chunk, &mem);
  Print("\n");
// #endif

  PrintStringTable(&chunk.symbols);

  DestroyMem(&mem);
  InitMem(&mem, 0);
  mem.symbols = chunk.symbols;

  VM vm;
  InitVM(&vm, &mem);
  Print("──╴Run╶──\n");
  RunChunk(&vm, &chunk);

  PrintMem(&mem);
}

int main(int argc, char *argv[])
{
  if (argc > 1) {
    if (StrEq(argv[1], "-h")) {
      Print("Usage: cassette [file]\n");
    } else {
      RunFile(argv[1]);
    }
  } else {
      Print("Usage: cassette [file]\n");
    // REPL(&mem);
  }
}
