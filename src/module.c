#include "module.h"
#include "env.h"
#include "reader.h"
#include "eval.h"
#include "mem.h"

Val LoadModule(char *path, Val base_env)
{
  // Val exp = ReadFile(path);
  // Val file_env = ExtendEnv(nil, nil, base_env);
  // EvalIn(exp, file_env);

  // DumpEnv(file_env);

  // Val frame = FirstFrame(file_env);
  // Val vars = FrameVars(frame);
  // Val vals = FrameVals(frame);
  // while (!IsNil(vars)) {
  //   Val val = Head(vals);
  //   if (IsEnv(val)) {
  //     Define(Head(vars), val, base_env);
  //   }
  //   vars = Tail(vars);
  //   vals = Tail(vals);
  // }

  return nil;
}
