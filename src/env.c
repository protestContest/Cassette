// #include "env.h"
// #include "base.h"
// #include "mem.h"
// #include "primitives.h"
// #include "printer.h"

// Val InitialEnv(void)
// {
//   Val prims = Primitives();
//   Val nil_sym = MakeSymbol("nil");
//   Val true_sym = MakeSymbol("true");
//   Val false_sym = MakeSymbol("false");

//   Val env = ExtendEnv(nil, Head(prims), Tail(prims));
//   Define(nil_sym, nil, env);
//   Define(true_sym, true_sym, env);
//   Define(false_sym, false_sym, env);
//   Define(MakeSymbol("MODULES"), MakeDict(nil, nil), env);

//   return env;
// }

// Val ExtendEnv(Val env, Val keys, Val vals)
// {
//   Val frame = MakeDict(keys, vals);
//   return MakePair(frame, env);
// }

// Val AddFrame(Val env, u32 size)
// {
//   Val frame = MakeDict(nil, nil);
//   return MakePair(frame, env);
// }

// void Define(Val name, Val value, Val env)
// {
//   Val frame = Head(env);
//   DictSet(frame, name, value);
// }

// void DefineModule(Val name, Val defs, Val env)
// {
//   Val modules = DictGet(Head(GlobalEnv(env)), SymbolFor("MODULES"));
//   DictSet(modules, name, defs);
// }

// Val GlobalEnv(Val env)
// {
//   while (!IsNil(Tail(env))) env = Tail(env);
//   return env;
// }

// EvalResult Lookup(Val var, Val env)
// {
//   if (Eq(var, MakeSymbol("ENV"))) {
//     return EvalOk(env);
//   }

//   Val cur_env = env;
//   while (!IsNil(cur_env)) {
//     Val frame = Head(cur_env);
//     if (DictHasKey(frame, var)) {
//       return EvalOk(DictGet(frame, var));
//     }

//     cur_env = Tail(cur_env);
//   }

//   DumpEnv(env, true);

//   char *msg = NULL;
//   PrintInto(msg, "Unbound variable \"%s\"", SymbolName(var));
//   return RuntimeError(msg);
// }

// void DumpFrame(Val frame)
// {
//   u32 var_size = 8;

//   for (u32 i = 0; i < DICT_BUCKETS; i++) {
//     Val bucket = TupleAt(frame, i);
//     while (!IsNil(bucket)) {
//       Val entry = Head(bucket);
//       Val var = Head(entry);
//       if (BinaryLength(var) > var_size) {
//         var_size = BinaryLength(var);
//       }
//       bucket = Tail(bucket);
//     }
//   }

//   for (u32 i = 0; i < DICT_BUCKETS; i++) {
//     Val bucket = TupleAt(frame, i);
//     while (!IsNil(bucket)) {
//       Val entry = Head(bucket);
//       Val var = Head(entry);
//       Val val = Tail(entry);
//       fprintf(stderr, "│ %-*s  %s\n", var_size, ValStr(var), ValStr(val));
//       bucket = Tail(bucket);
//     }
//   }
// }

// void DumpEnv(Val env, bool all)
// {
//   if (IsNil(env)) {
//     fprintf(stderr, "Env is nil\n");
//   } else {
//     fprintf(stderr, "┌╴Environment╶───────────\n");

//     while (!IsNil(Tail(env))) {
//       Val frame = Head(env);
//       DumpFrame(frame);
//       env = Tail(env);
//       fprintf(stderr, "├────────────────────────\n");
//     }

//     if (all) {
//       Val frame = Head(env);
//       DumpFrame(frame);
//     } else {
//       fprintf(stderr, "│ <base env>\n");
//     }


//     fprintf(stderr, "└────────────────────────\n");
//   }
// }
