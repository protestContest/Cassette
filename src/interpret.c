#include "interpret.h"
#include <univ/io.h>
#include <univ/sys.h>
#include <univ/vec.h>

#define DEBUG_EVAL

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
Val EvalDefine(Val exps, Val env, Mem *mem);
Val RuntimeError(char *message, Val exp, Mem *mem);
void PrintEnv(Val env, Mem *mem);

Val Interpret(Val ast, Mem *mem)
{
  if (IsNil(ast)) return nil;
  Val env = InitialEnv(mem);
  return Eval(ast, env, mem);
}

Val InitialEnv(Mem *mem)
{
  MakeSymbol(mem, "primitive");
  Val env = MakePair(mem, nil, nil);
  Define(env, MakeSymbol(mem, "+"), MakePair(mem, SymbolFor("primitive"), SymbolFor("+")), mem);
  Define(env, MakeSymbol(mem, "-"), MakePair(mem, SymbolFor("primitive"), SymbolFor("-")), mem);
  Define(env, MakeSymbol(mem, "*"), MakePair(mem, SymbolFor("primitive"), SymbolFor("*")), mem);
  Define(env, MakeSymbol(mem, "/"), MakePair(mem, SymbolFor("primitive"), SymbolFor("/")), mem);
  return env;
}

static u32 indent = 0;
Val Eval(Val exp, Val env, Mem *mem)
{
#ifdef DEBUG_EVAL
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Eval: ");
  PrintVal(mem, exp);
  // Print("\nEnv:\n");
  // PrintEnv(env, mem);
#endif

  indent++;
  Val result;

  if (IsNumeric(exp) || IsObj(exp)) {
    result = exp;
  } else if (IsSym(exp)) {
    result = Lookup(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("->"))) {
    result = MakeProc(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("do"))) {
    result = EvalBlock(exp, env, mem);
  } else if (IsTagged(mem, exp, SymbolFor("def"))) {
    result = EvalDefine(exp, env, mem);
  } else {
    result = EvalApply(exp, env, mem);
  }

  indent--;

#ifdef DEBUG_EVAL
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("=> ");
  PrintVal(mem, result);
  Print("\n");
#endif

  return result;
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
  while (!IsNil(env)) {
    Val frame = Head(mem, env);

    while (!IsNil(frame)) {
      Val item = Head(mem, frame);

      if (Eq(var, Head(mem, item))) {
        return Tail(mem, item);
      }

      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }

  return RuntimeError("Unbound variable", var, mem);
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
  return MakeList(mem, 4, MakeSymbol(mem, "proc"), params, body, env);
}

Val EvalApply(Val exp, Val env, Mem *mem)
{
  Val proc = Eval(Head(mem, exp), env, mem);
  Val args = EvalEach(Tail(mem, exp), env, mem);

  if (IsNumeric(proc) && IsNil(args)) {
    return proc;
  }

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

Val EvalDefine(Val exp, Val env, Mem *mem)
{
  Val var = ListAt(mem, exp, 1);
  if (!IsSym(var)) return RuntimeError("Define must be an ID", var, mem);

  Val val = Eval(ListAt(mem, exp, 2), env, mem);
  Define(env, var, val, mem);
  return val;
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

void PrintEnv(Val env, Mem *mem)
{
  if (IsNil(env)) Print("<empty env>");
  while (!IsNil(env)) {
    Print("----------------\n");
    Val frame = Head(mem, env);

    while (!IsNil(frame)) {
      Val item = Head(mem, frame);

      Val var = Head(mem, item);
      Val val = Tail(mem, item);
      Print(SymbolName(mem, var));
      Print(": ");
      PrintVal(mem, val);
      Print("\n");

      frame = Tail(mem, frame);
    }

    env = Tail(mem, env);
  }
}
