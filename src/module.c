#include "module.h"
#include "env.h"
#include "reader.h"
#include "eval.h"
#include "mem.h"
#include <dirent.h>

Val LoadFile(char *path)
{
  Reader *r = NewReader();
  ReadFile(r, path);

  if (r->status == PARSE_INCOMPLETE) {
    ParseError(r, "Unexpected end of file");
  }

  if (r->status == PARSE_ERROR) {
    PrintReaderError(r);
    return nil;
  }

  Val env = ExtendEnv(InitialEnv(), nil, nil);
  EvalResult result = Eval(r->ast, env);

  if (result.status != EVAL_OK) {
    PrintEvalError(result);
    return nil;
  } else {
    return env;
  }
}

Val FindModules(char *path)
{
  DIR *dir = opendir(path);
  if (dir == NULL) {
    fprintf(stderr, "Bad directory name\n");
  }


}
