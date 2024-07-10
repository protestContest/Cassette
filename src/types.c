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

#define IsTypeConst(t)    IsSym(t)
#define IsTypeVar(t)      IsInt(t)
#define IsTypeFunc(t)     IsTuple(t)
#define WhichTypeFunc(t)  (IsTuple(t) ? TupleGet(t, 0) : 0)
#define IsPolytype(t)     IsPair(t)

#define TypeError(msg, node)  MakeError(msg, NodeStart(node), NodeEnd(node))

val InferType(val node, val context);

val NewTypeVar(void);
val Polytype(val type, i32 boundVars);

val TypeOfVar(val var, val context);

static val Substitute(val a, val b, val type);
val Instantiate(val node, val polytype);
val ConstType(val node, char *t);

static i32 nextVar = 0;

val NewTypeVar(void)
{
  return IntVal(nextVar++);
}

val Polytype(val type, i32 boundVars)
{
  i32 i;
  val tvars = 0;
  if (IsPolytype(type)) return type;
  for (i = 0; i < boundVars; i++) {
    tvars = Pair(IntVal(nextVar - i - 1), tvars);
  }
  return Pair(type, tvars);
}

val FuncType(i32 nparams)
{
  val t = Tuple(nparams + 2);
  TupleSet(t, 0, SymVal(Symbol("fun")));
  return t;
}

val ConstType(val node, char *t)
{
  SetNodeProp(node, typeProp, SymVal(Symbol(t)));
  return 0;
}

val InferVar(val node, val context)
{
  val varType = TypeOfVar(NodeValue(node), context);
  if (!varType) return TypeError("Undefined variable", node);
  Instantiate(node, varType);
  return 0;
}

val InferPair(val node, val context)
{
  val head = NodeChild(node, 0);
  val tail = NodeChild(node, 1);
  val result = InferType(head, context);
  val t;

  if (IsError(result)) return result;
  result = InferType(tail, context);
  if (IsError(result)) return result;

  t = Tuple(3);
  TupleSet(t, 0, SymVal(Symbol("pair")));
  TupleSet(t, 1, NodeProp(head, typeProp));
  TupleSet(t, 2, NodeProp(tail, typeProp));
  SetNodeProp(node, typeProp, t);
  return 0;
}

val InferTuple(val node, val context)
{
  val items = NodeValue(node);
  val t = Tuple(ListLength(items) + 1);
  u32 i;
  TupleSet(t, 0, SymVal(Symbol("tuple")));
  for (i = 1; i < TupleLength(t); i++) {
    val item = Head(items);
    val result = InferType(item, context);
    if (IsError(result)) return result;
    items = Tail(items);
    TupleSet(t, i, NodeProp(item, typeProp));
  }
  SetNodeProp(node, typeProp, t);
  return 0;
}

val InferIf(val node, val context)
{
  val cons = NodeChild(node, 1);
  val alt = NodeChild(node, 2);
  val result = InferType(cons, context);
  if (IsError(result)) return result;
  result = InferType(alt, context);
  if (IsError(result)) return result;

  if (ValEq(NodeProp(cons, typeProp), NodeProp(alt, typeProp))) {
    SetNodeProp(node, typeProp, NodeProp(cons, typeProp));
  } else {
    val t = Tuple(3);
    TupleSet(t, 0, SymVal(Symbol("either")));
    TupleSet(t, 1, NodeProp(cons, typeProp));
    TupleSet(t, 2, NodeProp(alt, typeProp));
    SetNodeProp(node, typeProp, t);
  }
  return 0;
}

void PrintContext(val context)
{
  printf("Context:\n");
  while (context) {
    val entry = Head(context);
    printf("  %s: ", SymbolName(RawVal(Head(entry))));
    PrintType(Tail(entry));
    printf("\n");
    context = Tail(context);
  }
}

bool TypeInType(val needle, val haystack)
{
  if (IsTypeVar(haystack)) return needle == haystack;
  if (IsTypeFunc(haystack)) {
    u32 i;
    for (i = 0; i < TupleLength(haystack); i++) {
      if (TypeInType(needle, TupleGet(haystack, i))) return true;
    }
  }
  return false;
}

bool TypeInContext(val type, val context)
{
  while (context) {
    val entry = Head(context);
    val t = Tail(entry);
    if (TypeInType(type, t)) return true;
    context = Tail(context);
  }
  return false;
}

val FindFreeVars(val type, val context, val tvars)
{
  if (IsTypeConst(type)) return tvars;
  if (IsTypeVar(type) && !TypeInContext(type, context) && !InList(type, tvars)) {
    return Pair(type, tvars);
  }

  if (IsTypeFunc(type)) {
    u32 i;
    for (i = 1; i < TupleLength(type); i++) {
      tvars = FindFreeVars(TupleGet(type, i), context, tvars);
    }
  }
  return tvars;
}

val Generalize(val type, val context)
{
  return Pair(type, FindFreeVars(type, context, 0));
}

val InferDo(val node, val context)
{
  val stmts = Tail(NodeValue(node));
  val stmt, result;

  while (stmts) {
    stmt = Head(stmts);
    result = InferType(stmt, context);
    if (IsError(result)) return result;

    if (NodeType(stmt) == letNode) {
      val var = NodeValue(NodeChild(stmt, 0));
      val stmt_type = NodeProp(stmt, typeProp);
      context = Pair(Pair(var, Generalize(stmt_type, context)), context);
    }

    PrintAST(node);
    PrintContext(context);

    stmts = Tail(stmts);
  }
  SetNodeProp(node, typeProp, NodeProp(stmt, typeProp));
  return 0;
}

void SubNode(val t1, val t2, val node)
{
  val children;

  SetNodeProp(node, typeProp, Substitute(t1, t2, NodeProp(node, typeProp)));
  switch (NodeType(node)) {
  case nilNode:
  case idNode:
  case strNode:
  case symNode:
  case intNode:
    break;
  default:
    children = NodeValue(node);
    while (children) {
      SubNode(t1, t2, Head(children));
      children = Tail(children);
    }
  }
}

void SubContext(val t1, val t2, val context)
{
  while (context) {
    val entry = Head(context);
    val polytype = Tail(entry);
    val tvars = Tail(polytype);
    bool bound = false;
    while (tvars) {
      if (Head(tvars) == t1) {
        bound = true;
        break;
      }
      tvars = Tail(tvars);
    }
    if (!bound) SetHead(polytype, Substitute(t1, t2, Head(polytype)));
    context = Tail(context);
  }
}

bool SubSub(val t1, val t2, val subs)
{
  bool found = false;
  while (subs) {
    val entry = Head(subs);
    SetTail(entry, Substitute(t1, t2, Tail(entry)));
    if (t1 == Head(entry)) found = true;
    subs = Tail(subs);
  }
  return found;
}

void PrintSubs(val subs)
{
  printf("{");
  while (subs) {
    PrintType(Head(Head(subs)));
    printf(": ");
    PrintType(Tail(Head(subs)));
    subs = Tail(subs);
    if (subs) printf(", ");
  }
  printf("}");
}

val CombineSubs(val s1, val s2)
{
  val subs = s1;

  printf("Combine ");
  PrintSubs(s1);
  printf(" with ");
  PrintSubs(s2);
  printf(": ");

  while (s2) {
    val entry = Head(s2);
    if (!SubSub(Head(entry), Tail(entry), s1)) {
      subs = Pair(entry, subs);
    }
    s2 = Tail(s2);
  }
  PrintSubs(subs);
  printf("\n");
  return subs;
}

val ApplySubs(val subs, val type)
{
  while (subs) {
    val sub = Head(subs);
    subs = Tail(subs);
    type = Substitute(Head(sub), Tail(sub), type);
  }
  return type;
}

val Unify(val t1, val t2, val node, val context)
{
  val subs = 0;

  printf("  Unify ");
  PrintType(t1);
  printf(" with ");
  PrintType(t2);
  printf("\n");

  if (IsSym(t1) && t1 == t2) return 0;

  if (IsInt(t1) || IsInt(t2)) {
    if (t1 == t2) return 0;
    if (IsInt(t1)) {
      subs = Pair(Pair(t1, t2), 0);
      SubNode(t1, t2, node);
      SubContext(t1, t2, context);
    } else {
      subs = Pair(Pair(t2, t1), 0);
      SubNode(t2, t1, node);
      SubContext(t2, t1, context);
    }
    return subs;
  }

  if (IsTuple(t1) && IsTuple(t2) && TupleGet(t1, 0) == TupleGet(t2, 0)) {
    u32 i;
    val result;
    if (TupleLength(t1) != TupleLength(t2)) return TypeError("Wrong number of arguments", node);
    for (i = 1; i < TupleLength(t1); i++) {
      result = Unify(TupleGet(t1, i), TupleGet(t2, i), node, context);
      if (IsError(result)) return result;
      t1 = ApplySubs(result, t1);
      t2 = ApplySubs(result, t1);
      subs = CombineSubs(subs, result);
    }
    return subs;
  }

  return TypeError("Type mismatch", node);
}

void ApplySubsContext(val subs, val context)
{
  while (subs) {
    val sub = Head(subs);
    subs = Tail(subs);
    SubContext(Head(sub), Tail(sub), context);
  }
}

void ApplySubsNode(val subs, val node)
{
  while (subs) {
    val entry = Head(subs);
    SubNode(Head(entry), Tail(entry), node);
    subs = Tail(subs);
  }
}

val InferCall(val node, val context)
{
  val rt = NewTypeVar();
  val fun = NodeChild(node, 0);
  val args = Tail(NodeValue(node));
  u32 nargs = ListLength(args);
  val callType = FuncType(nargs);
  i32 i;
  val result;
  val subs = InferType(fun, context);
  if (IsError(subs)) return subs;

  ApplySubsContext(subs, context);

  i = 0;
  while (args) {
    result = InferType(Head(args), context);
    if (IsError(result)) return result;
    SetNodeProp(fun, typeProp, ApplySubs(result, NodeProp(fun, typeProp)));
    subs = CombineSubs(subs, result);
    TupleSet(callType, i+1, NodeProp(Head(args), typeProp));
    args = Tail(args);
    i++;
  }
  TupleSet(callType, nargs+1, rt);
  SetNodeProp(node, typeProp, rt);

  printf("Unify ");
  PrintType(NodeProp(fun, typeProp));
  printf(" with ");
  PrintType(callType);
  printf(" in \n  ");
  PrintNode(node, 1, 0);

  result = Unify(NodeProp(fun, typeProp), callType, node, context);
  if (IsError(result)) return result;

  subs = CombineSubs(subs, result);
  PrintAST(node);
  return subs;
}

val InferLambda(val node, val context)
{
  val params = NodeValue(NodeChild(node, 0));
  val body = NodeChild(node, 1);
  val t = FuncType(ListLength(params));
  i32 i = 0;
  val result;

  while (params) {
    val var = NodeValue(Head(params));
    val pt = NewTypeVar();
    SetNodeProp(Head(params), typeProp, pt);
    TupleSet(t, i+1, pt);
    context = Pair(Pair(var, Polytype(pt, 0)), context);
    params = Tail(params);
    i++;
  }
  result = InferType(body, context);
  if (IsError(result)) return result;

  TupleSet(t, TupleLength(t)-1, NodeProp(body, typeProp));
  t = ApplySubs(result, t);
  SetNodeProp(node, typeProp, t);
  ApplySubsNode(result, node);
  PrintAST(node);
  PrintSubs(result);
  printf("\n");

  return result;
}

val InferLet(val node, val context)
{
  val expr = NodeChild(node, 1);
  val result = InferType(expr, context);
  if (IsError(result)) return result;
  SetNodeProp(node, typeProp, NodeProp(expr, typeProp));
  return 0;
}

val InferModule(val node, val context)
{
  val result = InferType(ModNodeBody(node), context);
  if (IsError(result)) return result;
  return result;
}

val InferType(val node, val context)
{
  switch (NodeType(node)) {
  case nilNode:     return ConstType(node, "nil");
  case intNode:     return ConstType(node, "int");
  case symNode:     return ConstType(node, "sym");
  case strNode:     return ConstType(node, "str");
  case idNode:      return InferVar(node, context);
  case pairNode:    return InferPair(node, context);
  case tupleNode:   return InferTuple(node, context);
  case ifNode:      return InferIf(node, context);
  case doNode:      return InferDo(node, context);
  case callNode:    return InferCall(node, context);
  case lambdaNode:  return InferLambda(node, context);
  case letNode:     return InferLet(node, context);
  case moduleNode:  return InferModule(node, context);
  default:          return 0;
  }
}

val TypeOfVar(val var, val context)
{
  while (context) {
    val entry = Head(context);
    context = Tail(context);
    if (Head(entry) == var) return Tail(entry);
  }
  return 0;
}

static val Substitute(val a, val b, val type)
{
  if (IsInt(type) && type == a) return b;
  if (IsTuple(type)) {
    val t = Tuple(TupleLength(type));
    u32 i;
    TupleSet(t, 0, TupleGet(type, 0));
    for (i = 1; i < TupleLength(type); i++) {
      TupleSet(t, i, Substitute(a, b, TupleGet(type, i)));
    }
    type = t;
  }
  return type;
}

val Instantiate(val node, val polytype)
{
  val t = Head(polytype);
  val vars = Tail(polytype);
  while (vars) {
    t = Substitute(Head(vars), NewTypeVar(), t);
    vars = Tail(vars);
  }
  SetNodeProp(node, typeProp, t);
  return node;
}

void PrintType(val type)
{
  if (IsTypeConst(type)) {
    printf("%s", SymbolName(RawVal(type)));
  } else if (IsInt(type)) {
    printf("t%d", RawInt(type));
  } else if (IsTuple(type)) {
    char *typefn = SymbolName(RawVal(TupleGet(type, 0)));
    u32 i;
    printf("%s(", typefn);
    for (i = 0; i < TupleLength(type)-1; i++) {
      PrintType(TupleGet(type, i+1));
      if (i < TupleLength(type)-2) printf(", ");
    }
    printf(")");
  } else if (!type) {
    printf("??");
  } else if (IsPair(type)) {
    val tvars = Tail(type);
    printf("V");
    while (tvars) {
      PrintType(Head(tvars));
      tvars = Tail(tvars);
      if (tvars) printf(", ");
    }
    printf(". ");
    PrintType(Head(type));
  } else {
    printf("??");
  }
}

val BinOpType(char *op, char *a, char *b, char *c)
{
  val t = Tuple(4);
  TupleSet(t, 0, SymVal(Symbol("fun")));
  TupleSet(t, 1, SymVal(Symbol(a)));
  TupleSet(t, 2, SymVal(Symbol(b)));
  TupleSet(t, 3, SymVal(Symbol(c)));
  return Pair(SymVal(Symbol(op)), Polytype(t, 0));
}

val InferTypes(val node)
{
  val context = Pair(BinOpType("+", "int", "int", "int"), 0);
  val result = InferType(node, context);
  if (IsError(result)) return result;
  return node;
}
