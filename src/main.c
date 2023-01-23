#include "reader.h"
#include "inst.h"
#include "compile.h"
#include "mem.h"

int main(int argc, char *argv[])
{
  Reader *r = NewReader();
  ReadFile(r, "test.rye");

  if (r->status == PARSE_INCOMPLETE) {
    ParseError(r, "Unexpected end of file");
  }

  if (r->status == PARSE_ERROR) {
    PrintReaderError(r);
    exit(1);
  }

  Disassemble("Result", Compile(Second(r->ast), REG_VAL, SymbolFor("return")));

  // if (argc > 1) {
  //   RunProject(argv[1]);
  // } else {
  //   Val env = RunProject(NULL);
  //   REPL(env);
  // }
}
