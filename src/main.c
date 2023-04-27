#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "chunk.h"
#include <univ/window.h>

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
  Val ast = Parse(src, &mem);
  Val compiled = Compile(ast, &mem);
  Chunk chunk = Assemble(compiled, &mem);

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);
}

// void REPL(Mem *mem)
// {
//   char line[1024];
//   Source src = {"repl", line, 0};

//   VM vm;
//   InitVM(&vm, mem);

//   while (true) {
//     Print("> ");
//     if (!ReadLine(line, 1024)) break;
//     src.length = StrLen(line);

//     Val ast = Parse(src, mem);
//     PrintVal(mem, Eval(ast, &vm));
//     Print("\n");

//     HandleEvents();
//   }
// }

int main(int argc, char *argv[])
{
  Seed(Microtime());

  if (argc > 1) {
    RunFile(argv[1]);
  } else {
    Print("Usage: cassette <file>\n");
    // REPL(&mem);
  }
}
