#include "env.h"
#include "value.h"
#include "module.h"
#include "eval.h"
#include "printer.h"
#include "repl.h"
#include "mem.h"

int ExecuteScript(char *path);

int main(int argc, char *argv[])
{
  if (argc > 1) {
    return ExecuteScript(argv[1]);
  } else {
    ExecuteScript("test.rye");
    // REPL();
  }
}

int ExecuteScript(char *path)
{
  Val env = LoadFile(path);
  DumpEnv(env);
  // Val frame = Head(env);
  // for (u32 i = 0; i < DICT_BUCKETS; i++) {
  //   PrintVal(TupleAt(frame, i));
  // }
  return 0;
}
