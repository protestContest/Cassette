#include "interpret.h"
#include <univ/io.h>
#include <univ/sys.h>
#include <univ/vec.h>

Val InitialEnv(Mem *mem);
Val Eval(Val exp, Val env, Mem *mem);
Val Apply(Val exp, Val env, Mem *mem);
Val ApplyPrimitive(Val proc, Val args, Mem *mem);
Val ExtendEnv(Val env, Val vars, Val vals, Mem *mem);
Val Lookup(Val exp, Val env, Mem *mem);
void Define(Val env, Val var, Val value, Mem *mem);
Val MakeProc(Val exp, Val env, Mem *mem);
Val EvalApply(Val exp, Val env, Mem *mem);
Val EvalEach(Val exps, Val env, Mem *mem);
Val EvalBlock(Val exps, Val env, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);

Val Interpret(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  Val env = InitialEnv(mem);
  return Eval(ast, env, mem);
}

Val InitialEnv(Mem *mem)
{
  Val vars = MakeList(mem, 4,
                      SymbolFor("+"),
                      SymbolFor("-"),
                      SymbolFor("*"),
                      SymbolFor("/"));
  Val vals = MakeList(mem, 4,
                      MakePair(mem, SymbolFor("primitive"), SymbolFor("+")),
                      MakePair(mem, SymbolFor("primitive"), SymbolFor("-")),
                      MakePair(mem, SymbolFor("primitive"), SymbolFor("*")),
                      MakePair(mem, SymbolFor("primitive"), SymbolFor("/")));
  Val env = MakePair(mem, nil, nil);
  env = ExtendEnv(env, vars, vals, mem);
  return env;
}

Val Eval(Val exp, Val env, Mem *mem)
{
  if (IsNumeric(exp) || IsObj(exp)) return exp;
  if (IsSym(exp)) return Lookup(exp, env, mem);
  if (IsTagged(mem, exp, SymbolFor("->"))) return MakeProc(exp, env, mem);
  if (IsTagged(mem, exp, SymbolFor("do"))) return EvalBlock(exp, env, mem);
  return EvalApply(exp, env, mem);
}

Val Apply(Val proc, Val args, Mem *mem)
{
  if (IsTagged(mem, proc, SymbolFor("primitive"))) {
    return ApplyPrimitive(proc, args, mem);
  }

  if (!IsTagged(mem, proc, SymbolFor("proc"))) {
    return RuntimeError("Not a procedure", proc, mem);
  }
  Val params = ListAt(mem, proc, 1);
  Val body = ListAt(mem, proc, 2);
  Val env = ExtendEnv(ListAt(mem, proc, 3), params, args, mem);

  return Eval(body, env, mem);
}

Val ApplyPrimitive(Val proc, Val args, Mem *mem)
{
  Val op = Tail(mem, proc);
  if (Eq(op, SymbolFor("+"))) {
    if (ListLength(mem, args) != 2) RuntimeError("Wrong number of args for '+'", args, mem);
    Val a = ListAt(mem, args, 0);
    Val b = ListAt(mem, args, 1);
    if (IsInt(a) && IsInt(b)) {
      return IntVal(RawInt(a) + RawInt(b));
    } else if (IsInt(a)) {
      return NumVal(RawInt(a) + RawNum(b));
    } else if IsInt(b) {
      return NumVal(RawNum(a) + RawInt(b));
    } else {
      return NumVal(RawNum(a) + RawNum(b));
    }
  } else if (Eq(op, SymbolFor("-"))) {
    if (ListLength(mem, args) != 2) RuntimeError("Wrong number of args for '-'", args, mem);
    Val a = ListAt(mem, args, 0);
    Val b = ListAt(mem, args, 1);
    if (IsInt(a) && IsInt(b)) {
      return IntVal(RawInt(a) - RawInt(b));
    } else if (IsInt(a)) {
      return NumVal(RawInt(a) - RawNum(b));
    } else if IsInt(b) {
      return NumVal(RawNum(a) - RawInt(b));
    } else {
      return NumVal(RawNum(a) - RawNum(b));
    }
  } else if (Eq(op, SymbolFor("*"))) {
    if (ListLength(mem, args) != 2) RuntimeError("Wrong number of args for '*'", args, mem);
    Val a = ListAt(mem, args, 0);
    Val b = ListAt(mem, args, 1);
    if (IsInt(a) && IsInt(b)) {
      return IntVal(RawInt(a) * RawInt(b));
    } else if (IsInt(a)) {
      return NumVal(RawInt(a) * RawNum(b));
    } else if IsInt(b) {
      return NumVal(RawNum(a) * RawInt(b));
    } else {
      return NumVal(RawNum(a) * RawNum(b));
    }
  } else if (Eq(op, SymbolFor("/"))) {
    if (ListLength(mem, args) != 2) RuntimeError("Wrong number of args for '/'", args, mem);
    Val a = ListAt(mem, args, 0);
    Val b = ListAt(mem, args, 1);
    if (IsInt(a) && IsInt(b)) {
      return NumVal((float)RawInt(a) / RawInt(b));
    } else if (IsInt(a)) {
      return NumVal(RawInt(a) / RawNum(b));
    } else if IsInt(b) {
      return NumVal(RawNum(a) / RawInt(b));
    } else {
      return NumVal(RawNum(a) / RawNum(b));
    }
  }

  return RuntimeError("Unimplemented primitive", proc, mem);
}

Val ExtendEnv(Val env, Val vars, Val vals, Mem *mem)
{
  env = MakePair(mem, nil, env);
  while (!IsNil(vars)) {
    Define(env, Head(mem, vars), Head(mem, vals), mem);
    vars = Tail(mem, vars);
    vals = Tail(mem, vals);
  }

  return env;
}

Val Lookup(Val var, Val env, Mem *mem)
{
  if (IsNil(env)) {
    return RuntimeError("Unbound variable", var, mem);
  }
  Val frame = Head(mem, env);
  while (!IsNil(frame)) {
    Val item = Head(mem, frame);
    if (Eq(Head(mem, item), var)) return Tail(mem, item);
    frame = Tail(mem, frame);
  }

  return Lookup(var, Tail(mem, env), mem);
}

void Define(Val env, Val var, Val value, Mem *mem)
{
  Val frame = Head(mem, env);
  if (IsNil(frame)) {
    Val new_item = MakePair(mem, var, value);
    frame = MakePair(mem, new_item, nil);
    SetHead(mem, env, frame);
    return;
  }

  while (true) {
    Val item = Head(mem, frame);
    if (Eq(Head(mem, item), var)) {
      SetTail(mem, item, value);
      return;
    }

    if (IsNil(Tail(mem, frame))) {
      Val new_item = MakePair(mem, var, value);
      SetTail(mem, frame, MakePair(mem, new_item, nil));
      return;
    }
    frame = Tail(mem, frame);
  }
}

Val MakeProc(Val exp, Val env, Mem *mem)
{
  Val params = ListAt(mem, exp, 1);
  Val body = ListAt(mem, exp, 2);
  return MakeList(mem, 4, SymbolFor("proc"), params, body, env);
}

Val EvalApply(Val exp, Val env, Mem *mem)
{
  Val proc = Eval(Head(mem, exp), env, mem);
  Val args = EvalEach(Tail(mem, exp), env, mem);
  return Apply(proc, args, mem);
}

Val EvalEach(Val exps, Val env, Mem *mem)
{
  Val vals = nil;
  while (!IsNil(exps)) {
    Val val = Eval(Head(mem, exps), env, mem);
    vals = MakePair(mem, val, vals);
    exps = Tail(mem, exps);
  }
  return ReverseOnto(mem, vals, nil);
}

Val EvalBlock(Val exps, Val env, Mem *mem)
{
  Val block = Tail(mem, exps);
  Val result = nil;
  while (!IsNil(block)) {
    result = Eval(Head(mem, block), env, mem);
    block = Tail(mem, block);
  }
  return result;
}

Val RuntimeError(char *message, Val exp, Mem *mem)
{
  Print(message);
  Print(": ");
  PrintVal(mem, exp);
  Print("\n");
  Exit();
  return nil;
}
