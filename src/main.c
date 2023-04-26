#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "eval.h"
#include "assemble.h"
#include <univ/window.h>

void REPL(Mem *mem)
{
  char line[1024];
  Source src = {"repl", line, 0};

  VM vm;
  InitVM(&vm, mem);

  while (true) {
    Print("> ");
    if (!ReadLine(line, 1024)) break;
    src.length = StrLen(line);

    Val ast = Parse(src, mem);
    PrintVal(mem, Eval(ast, &vm));
    Print("\n");

    HandleEvents();
  }
}

int main(int argc, char *argv[])
{
  Mem mem;
  InitMem(&mem, 0);

  char *data = (char*)ReadFile("test.cassette");

  if (!data) {
    Print("Could not open file");
    Exit();
  }

  Source src = {"test.cassette", data, StrLen(data)};

  Val ast = Parse(src, &mem);
  PrintAST(ast, &mem);
  Val compiled = Compile(ast, &mem);
  Print("\n\n--Compiled--\n");
  PrintSeq(compiled, &mem);

  Chunk chunk = Assemble(compiled, &mem);

  for (u32 i = 0; i < 7; i++) {
    PrintInt(i);
    Print(" ");
    PrintReg(IntToReg(i));
    Print("\n");
  }

  for (u32 i = 0; i < 18; i++) {
    PrintInt(i);
    Print(" ");
    PrintOpCode(i);
    Print("\n");
  }

  Print("\n--Chunk--\n");
  PrintChunkConstants(&chunk, &mem);
  HexDump(chunk.data, VecCount(chunk.data), 32);

  Print("\n--Assembled--\n");
  PrintChunk(&chunk, &mem);

  // if (argc > 1) {
  //   RunFile(argv[1], &mem);
  // } else {
  //   REPL(&mem);
  // }
}
