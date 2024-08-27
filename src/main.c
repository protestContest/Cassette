#include "result.h"
#include "project.h"
#include "program.h"
#include "vm.h"
#include "univ/file.h"
#include "univ/str.h"

int main(int argc, char *argv[])
{
  Project *project;
  Program *program;
  char *searchpath = 0;
  Result result;

  if (argc < 2) {
    fprintf(stderr, "Source file required\n");
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
    FreeError(result.data.p);
    FreeProject(project);
    return 1;
  }
  program = result.data.p;
  FreeProject(project);

#if DEBUG
  DisassembleProgram(program);
  fprintf(stderr, "\n");

  {
    u8 *bytes = 0;
    u32 size;
    size = SerializeProgram(program, &bytes);
    HexDump(bytes, size, 0);
    fprintf(stderr, "\n");
    WriteFile(bytes, size, "test.csst");
    free(bytes);
  }
#endif

  result = VMRun(program);
  if (!result.ok) {
    PrintError(result);
    PrintStackTrace(result);
    FreeStackTrace(result);
    FreeError(result.data.p);
  }

  FreeProgram(program);
  return 0;
}
