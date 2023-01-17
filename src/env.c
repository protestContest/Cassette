#include "env.h"
#include "mem.h"
#include "primitives.h"
#include "printer.h"

Val InitialEnv(void)
{
  Val prims = Primitives();
  Val keys = MakePair(MakeSymbol("nil"), MakePair(MakeSymbol("true"), MakePair(MakeSymbol("false"), Head(prims))));
  Val vals = MakePair(nil, MakePair(MakeSymbol("true"), MakePair(MakeSymbol("false"), Tail(prims))));
  return ExtendEnv(nil, keys, vals);;
}

Val ExtendEnv(Val env, Val keys, Val vals)
{
  Val frame = MakeDict(keys, vals);
  return MakePair(frame, env);
}

Val AddFrame(Val env, u32 size)
{
  Val frame = MakeEmptyDict(size);
  return MakePair(frame, env);
}

EvalResult Lookup(Val var, Val env)
{
  if (Eq(var, MakeSymbol("ENV"))) {
    return EvalOk(env);
  }

  Val cur_env = env;
  while (!IsNil(cur_env)) {
    Val frame = Head(cur_env);
    if (DictHasKey(frame, var)) {
      return EvalOk(DictGet(frame, var));
    }

    cur_env = Tail(cur_env);
  }

  DumpEnv(env);

  char *msg = NULL;
  PrintInto(msg, "Unbound variable \"%s\"", SymbolName(var));
  return RuntimeError(msg);
}

void DumpEnv(Val env)
{
  if (IsNil(env)) {
    fprintf(stderr, "Env is nil\n");
  } else {
    fprintf(stderr, "┌────────────────────────\n");

    while (!IsNil(Tail(env))) {
      Val frame = Head(env);

      for (u32 i = 0; i < DictSize(frame); i++) {
        Val var = DictKeyAt(frame, i);
        Val val = DictValueAt(frame, i);

        fprintf(stderr, "│ %s: %s\n", ValStr(var), ValStr(val));
      }

      env = Tail(env);
      fprintf(stderr, "├────────────────────────\n");
    }

    fprintf(stderr, "│ <base env>\n");
    fprintf(stderr, "└────────────────────────\n");
  }
}
