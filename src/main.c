#include "reader.h"
#include "chunk.h"
#include "compile.h"
#include "mem.h"
#include "printer.h"
#include "vm.h"

int main(int argc, char *argv[])
{
  Chunk *chunk = MakeChunk(7,
    OP_ASSIGN, IntVal(24), REG_VAL,
    OP_LABEL, MakeSymbol("test"),
    OP_TEST, IntVal(0), REG_VAL,
    OP_BRANCH, SymbolFor("done"),
    OP_ADD, IntVal(-1), REG_VAL,
    OP_GOTO, SymbolFor("test"),
    OP_LABEL, MakeSymbol("done"));

  Assemble(chunk);
  PrintChunk("Chunk", chunk);
  printf("\n");

  VM *vm = NewVM();
  vm->trace = true;
  Run(vm, chunk);

  // Chunk linkage = CompileLinkage(MakeSymbol("return"));
  // Disassemble("linkage", linkage);
}

// int main(int argc, char *argv[])
// {
  // Reader *r = NewReader();
  // ReadFile(r, "test.rye");

  // if (r->status == PARSE_INCOMPLETE) {
  //   ParseError(r, "Unexpected end of file");
  // }

  // if (r->status == PARSE_ERROR) {
  //   PrintReaderError(r);
  //   exit(1);
  // }

  // Disassemble("Result", Compile(Second(r->ast), REG_VAL, SymbolFor("return")));
// }


// int main(int argc, char *argv[])
// {
  // if (argc > 1) {
  //   RunProject(argv[1]);
  // } else {
  //   Val env = RunProject(NULL);
  //   REPL(env);
  // }
// }
