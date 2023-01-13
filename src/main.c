#include "env.h"
#include "value.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"
#include "repl.h"

int ExecuteScript(char *path);

int main(int argc, char *argv[])
{
  ExecuteScript("test.rye");
  // if (argc > 1) {
  //   return ExecuteScript(argv[1]);
  // } else {
  //   REPL();
  // }
}

int ExecuteScript(char *path)
{
  Reader *reader = NewReader();
  ReadFile(reader, path);

  switch (reader->status) {
  case PARSE_OK:
    Eval(reader->ast);
    break;
  case PARSE_INCOMPLETE:
    ParseError(reader, "Unexpected end of input");
  case PARSE_ERROR:
    PrintReaderError(reader);
  }
  return reader->status;
}
