#include "repl.h"
#include "reader.h"
#include "eval.h"
#include "mem.h"
#include "env.h"
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

void REPL(Val env)
{
  char line[BUFSIZ];
  reader = NewReader();
  reader->file = "repl";

  env = ExtendEnv(env, nil, nil);

  signal(SIGINT, &OnSignal);

  Prompt(reader);
  fgets(line, BUFSIZ, stdin);
  while (!feof(stdin)) {
    Read(reader, line);

    if (reader->status == PARSE_OK) {
      Val exp = ListLast(reader->ast);
      EvalResult result = Eval(exp, env);
      if (result.status == EVAL_OK) {
        fprintf(stderr, "=> %s\n", ValStr(result.value));
      } else {
        PrintEvalError(result);
      }
    } else if (reader->status == PARSE_ERROR) {
      PrintReaderError(reader);
    }

    Prompt(reader);
    fgets(line, BUFSIZ, stdin);
  }
}
