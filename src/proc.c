// #include "proc.h"
// #include "mem.h"

// Val MakeProc(Val name, Val params, Val body, Val env)
// {
//   return MakeTuple(5, MakeSymbol("procedure"), name, params, body, env);
// }

// bool IsProc(Val proc)
// {
//   return IsTagged(proc, "primitive") || IsTagged(proc, "procedure");
// }

// Val ProcName(Val proc)
// {
//   return TupleAt(proc, 1);
// }

// Val ProcParams(Val proc)
// {
//   Val params = TupleAt(proc, 2);
//   if (IsList(params)) {
//     return params;
//   } else {
//     return MakePair(params, nil);
//   }
// }

// Val ProcBody(Val proc)
// {
//   return TupleAt(proc, 3);
// }

// Val ProcEnv(Val proc)
// {
//   return TupleAt(proc, 4);
// }
