#include "interpret.h"
#include "mem.h"
#include "print.h"
#include <univ/io.h>
#include <stdlib.h>

Val Eval(ASTNode *node, Val env, Val *heap);
Val Apply(ASTNode *call, Val env, Val *heap);

static Val ProcParams(Val proc, Val *heap)
{
  return First(heap, proc);
}

static Val ProcEnv(Val proc, Val *heap)
{
  return Second(heap, proc);
}

static Val ExtendEnv(Val env, Val *heap)
{
  return MakePair(&heap, nil, env);
}

static void Define(Val env, Val var, Val value, Val *heap)
{
  Val frame = Head(heap, env);
  while (!IsNil(frame)) {
    Val item = Head(heap, frame);
    if (Eq(Head(heap, item), var)) {
      SetTail(&heap, item, value);
      return;
    }
    if (IsNil(Tail(heap, frame))) {
      break;
    }
    frame = Tail(heap, frame);
  }

  Val item = MakePair(&heap, var, value);
  SetTail(&heap, frame, item);
}

static void EvalArgs(ASTNode *node, Val env, Val *heap, Val *args)
{
  for (u32 i = 0; i < node->length; i++) {
    args[i] = Eval(node->children[i], env, heap);
  }
}

static Val ApplyPrimitive(char op, ASTNode *node, Val env, Val *heap)
{
  Val a = Eval(node->children[0], env, heap);
  Val b = Eval(node->children[1], env, heap);

  if (!IsNumeric(a) || !IsNumeric(b)) {
    Print("Bad arguments to '");
    PrintInt(op, 0);
    Print("'\n");
    exit(1);
  }

  Val result = nil;
  if (IsInt(a) && IsInt(b) && op != '/') {
    switch (op) {
    case '+': result = IntVal(RawInt(a) + RawInt(b)); break;
    case '-': result = IntVal(RawInt(a) - RawInt(b)); break;
    case '*': result = IntVal(RawInt(a) * RawInt(b)); break;
    }
  } else {
    float raw_a = IsInt(a) ? (float)RawInt(a) : RawNum(a);
    float raw_b = IsInt(b) ? (float)RawInt(b) : RawNum(b);

    switch (op) {
    case '+': result = NumVal(raw_a + raw_b); break;
    case '-': result = NumVal(raw_a - raw_b); break;
    case '*': result = NumVal(raw_a * raw_b); break;
    case '/': result = NumVal(raw_a / raw_b); break;
    }
  }

  return result;
}

static Val Lookup(Val var, Val env, Val *heap)
{
  if (IsNil(env)) return nil;

  Val frame = Head(heap, env);
  while (!IsNil(frame)) {
    Val item = Head(heap, frame);
    if (Eq(Head(heap, item), var)) {
      return Tail(heap, item);
    }
    frame = Tail(heap, frame);
  }

  return Lookup(var, Tail(heap, env), heap);
}

Val Eval(ASTNode *node, Val env, Val *heap)
{
  if (node->symbol == ParseSymNum) return node->token.value;
  if (node->symbol == ParseSymPlus) return ApplyPrimitive('+', node, env, heap);
  if (node->symbol == ParseSymMinus) return ApplyPrimitive('-', node, env, heap);
  if (node->symbol == ParseSymStar) return ApplyPrimitive('*', node, env, heap);
  if (node->symbol == ParseSymSlash) return ApplyPrimitive('/', node, env, heap);
  if (node->symbol == ParseSymID) return Lookup(node->token.value, env, heap);
  // if (node->symbol == ParseSymArrow) return MakeProcedure(node, heap);
  // if (node->symbol == ParseSymCall) return Apply(node, env, heap);

  Print("Runtime error\n");
  exit(1);
}

Val Interpret(ASTNode *node, Val *heap)
{
  Val env = ExtendEnv(nil, heap);
  return Eval(node, env, heap);
}

// Val Apply(ASTNode *call, Val env, Val *heap)
// {
//   Val args[call->length];
//   EvalArgs(call, env, heap, args);

//   Val proc = args[0];
//   Val proc_env = ExtendEnv(ProcEnv(proc, heap), heap);
//   Val params = ProcParams(proc, heap);
//   u32 i = 0;
//   while (!IsNil(params)) {
//     Define(proc_env, Head(heap, params), args[i], heap);
//     params = Tail(heap, params);
//     i++;
//   }


// }


// static Val MakeProcedure(ASTNode *node, Val *heap)
// {
//   ASTNode *params = node->children[0];
//   MakeList(&heap, 3, SymbolFor("procedure"))
// }