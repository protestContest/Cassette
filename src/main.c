#include "compile.h"
#include "vm.h"
#include "parse.h"
#include "ast.h"
#include "chunk.h"

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
  Val compiled = Compile(Tail(&mem, parsed), &mem);
  Chunk chunk = Assemble(compiled, &mem);

  Disassemble(&chunk, &mem);
  Print("\n");

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);
}

// void REPL(void)
// {
//   char line[1024];
//   Source src = {"repl", line, 0};

//   Mem mem;
//   InitMem(&mem, 0);

//   VM vm;
//   InitVM(&vm, &mem);

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
