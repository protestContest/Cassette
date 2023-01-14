#include "repl.h"
#include "reader.h"
#include "eval.h"
#include "printer.h"

Reader *reader = NULL;

void OnSignal(int sig)
{
  if (reader->status == PARSE_INCOMPLETE) {
    CancelRead(reader);
  } else {
    exit(0);
  }
}

void ExitSignal(int sig)
{

}

void Prompt(Reader *reader)
{
  if (reader->status == PARSE_INCOMPLETE) {
    printf("  ");
  } else {
    printf("? ");
  }
}

void REPL(void)
{
  char line[BUFSIZ];
  reader = NewReader();
  reader->file = "repl";

  signal(SIGINT, &OnSignal);

  Prompt(reader);
  fgets(line, BUFSIZ, stdin);
  while (!feof(stdin)) {
    Read(reader, line);

    if (reader->status == PARSE_OK) {
      // TODO: Eval
      PrintVal(reader->ast);
      PrintTree(reader->ast);
    } else if (reader->status == PARSE_ERROR) {
      PrintReaderError(reader);
    }

    Prompt(reader);
    fgets(line, BUFSIZ, stdin);
  }
}
