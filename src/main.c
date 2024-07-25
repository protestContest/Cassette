#include "result.h"
#include "project.h"
#include "program.h"
#include "vm.h"
#include <univ/str.h>
#include <univ/debug.h>

int main(int argc, char *argv[])
{
  Project *project;
  Program *program;
  u8 *bytes = 0;
  u32 size;
  char *searchpath = 0;
  Result result;

  if (argc < 2) {
    fprintf(stderr, "%sSource file required%s\n", ANSIRed, ANSINormal);
    return 1;
  }
  if (argc >= 3) {
    searchpath = argv[2];
  } else {
    searchpath = getenv("CASSETTE_PATH");
  }

  project = NewProject(argv[1], searchpath);
  result = BuildProject(project);
  if (IsError(result)) {
    PrintError(result);
    return 1;
  }
  program = result.data.p;

  Disassemble(program->code);
  fprintf(stderr, "\n");

  size = SerializeProgram(program, &bytes);
  HexDump(bytes, size);
  fprintf(stderr, "\n");

  VMDebug(program);
}
