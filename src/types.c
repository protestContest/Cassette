#include "types.h"
#include "parse.h"
#include "primitives.h"
#include "univ/symbol.h"

/*
Types can be:
- Type constant (symbols "int", "str", "sym", "nil")
- Type variable (integer)
- Function type (tuple {:fun, ...param types, return type})
- Pair type (tuple {:pair, head type, tail type})
- Tuple type (tuple {:tuple, ...item types})
- Polytype (pair of [type, bound var (integer)])

A substitution is a list of (a -> b) pairs

A type judgement is a pair of (type, substitution)

A context is a list of (var, type) pairs

A failure is a type judgement where the type is :error

Expressions are typed as follows:

numbers: const :int
strings: const :str
symbols: const :sym
nil: const :nil
var: lookup type in env
lambda: {:fun, ...param types, return type}
do:
*/

#define IsTypeConst(t)  IsSym(t)
#define IsTypeVar(t)    IsInt(t)
#define IsTypeFunc(t)   IsTuple(t)
#define IsPolytype(t)   IsPair(t)

#define TypeError(msg, node)    Pair(SymVal(Symbol("error")), MakeError(msg, NodeStart(node)))
#define IsTypeError(result)     (Head(result) == SymVal(Symbol("error")))

static val InferType(val node, val context);

static val NewType(void)
{
  static i32 nextVar = 0;
  return IntVal(nextVar++);
}

static val Polytype(val type)
{
  if (IsPolytype(type)) return type;
  return Pair(type, 0);
}

static val FuncType(val param, val result)
{
  val t = Tuple(3);
  TupleSet(t, 0, SymVal(Symbol("->")));
  TupleSet(t, 1, param);
  TupleSet(t, 2, result);
  return t;
}

static val Substitute(val a, val b, val type)
{
  if (IsSym(type)) return type;
  if (IsInt(type)) return (type == a) ? b : a;
  if (IsTuple(type)) {
    if (TupleGet(type, 0) == SymVal(Symbol("->"))) {
      val param = TupleGet(type, 1);
      val result = TupleGet(type, 2);
      if (param == a) return a;
      result = Substitute(a, b, result);
      return FuncType(param, result);
    }
  }

  return 0;
}

static val Instantiate(val sigma)
{
  val t = Head(sigma);
  val vars = Tail(sigma);
  while (vars) {
    t = Substitute(Head(vars), NewType(), t);
    vars = Tail(vars);
  }
  return t;
}

static val TypeConst(val node, char *t)
{
  SetNodeProp(node, type, Pair(SymVal(Symbol(t)), 0));
  return NodeProp(node, type);
}

static val InferVar(val node, val context)
{
  while (context) {
    val entry = Head(context);
    if (Head(entry) == NodeValue(node)) {
      val t = Instantiate(Tail(entry));
      SetNodeProp(node, type, t);
      return t;
    }
    context = Tail(context);
  }
  return TypeError("Type: Undefined variable", node);
}

static val InferDo(val node, val context)
{
  val stmts = Tail(NodeValue(node));
  val result;
  while (stmts) {
    result = InferType(Head(stmts), context);
    if (IsTypeError(result)) return result;
    stmts = Tail(stmts);
  }
  return result;
}

static val InferCall(val node, val context)
{
  val fn = NodeChild(node, 0);
  val arg = NodeChild(node, 1);
  val result = InferType(fn, context);
  if (IsTypeError(result)) return result;
  result = InferType(arg, context);
  if (IsTypeError(result)) return result;
}

static val InferType(val node, val context)
{
  printf("---\n");
  PrintAST(node);

  switch (NodeType(node)) {
  case nilNode:
    return TypeConst(node, "nil");
  case intNode:
    return TypeConst(node, "int");
  case symNode:
    return TypeConst(node, "sym");
  case strNode:
    return TypeConst(node, "str");
  case idNode:
    return InferVar(node, context);
  case doNode:
    return InferDo(node, context);
  case callNode:
    return InferCall(node, context);
  }

  return TypeError("Unimplemented", node);
}

static val TypeEnv(val env)
{
  val types = 0;
  while (env) {
    val frame = Head(env);
    u32 i;
    for (i = 0; i < TupleLength(frame); i++) {
      val id = TupleGet(frame, i);
      val entry = Pair(id, Polytype(NewType()));
      types = Pair(entry, types);
    }
    env = Tail(env);
  }
  return types;
}

val InferTypes(val ast)
{
  val expr, result;
  val env = TypeEnv(PrimitiveEnv());
  expr = (NodeType(ast) == moduleNode) ? NodeChild(ast, 3) : ast;
  result = InferType(expr, env);
  if (IsTypeError(result)) return Tail(result);
  return ast;
}

void PrintType(val type)
{
  if (IsTypeConst(type)) {
    printf("%s", SymbolName(RawVal(type)));
  } else if (IsInt(type)) {
    printf("t%d", RawInt(type));
  } else if (IsTuple(type)) {
    PrintType(TupleGet(type, 1));
    printf(" -> ");
    PrintType(TupleGet(type, 2));
  } else {
    printf("??");
  }
}
