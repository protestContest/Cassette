#include "types.h"
#include "parse.h"
#include "lex.h"
#include "primitives.h"
#include "node.h"
#include "error.h"
#include <univ/symbol.h>
#include <univ/str.h>

/*
Types can be:
- Type constant (symbols "int", "str", "sym", "nil")
- Type variable (integer)
- Type function (tuple, where first element is function name):
  - Function type ({:fn, ...param types, return type})
  - Pair type ({:pair, head type, tail type})
  - Tuple type ({:tuple, ...item types})
- Polytype (list of [type, ...type vars])
- A type error (nil)

A substitution is a list of (type var, type) pairs
A context is a list of (var, type) pairs
*/

#define IsTypeConst(t)        IsSym(t)
#define IsTypeVar(t)          IsInt(t)
#define IsTypeFunc(t)         IsTuple(t)
#define IsPolytype(t)         IsPair(t)
#define TypeError(msg, node)  \
  MakeError(Binary(msg), Pair(NodeStart(node), NodeEnd(node)))

#define ConstType(name)       SymVal(Symbol(name))
#define PolyType(type, tvars) Pair(type, tvars)

/* returns a new, unique type variable */
val NewTypeVar(void)
{
  static i32 nextVar = 0;
  return IntVal(nextVar++);
}

/* creates a type function */
val TypeFunc(char *name, i32 nparams)
{
  val t = Tuple(nparams + 1);
  TupleSet(t, 0, SymVal(Symbol(name)));
  return t;
}

val Either(val t1, val t2)
{
  val type = TypeFunc("either", 2);
  TupleSet(type, 1, t1);
  TupleSet(type, 2, t2);
  return type;
}

/* returns the type of a variable in the context */
val TypeOfVar(val var, val context)
{
  while (context) {
    val entry = Head(context);
    context = Tail(context);
    if (Head(entry) == var) return Tail(entry);
  }
  return 0;
}

/* returns whether a type appears in another type */
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

void PrintType(val type)
{
  if (IsTypeConst(type)) {
    printf("%s", SymbolName(RawVal(type)));
  } else if (IsInt(type)) {
    i32 suffix = RawInt(type) / 26;
    i32 letter = RawInt(type) % 26;
    printf("%c", letter + 'a');
    if (suffix) printf("%d", suffix);
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

/*** CONTEXT ***/

/* returns whether a type appears in a context */
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

/*** SUBSTITUTION ***/

/* A substitution is a set of {type var: type} replacement pairs. */

/* returns a copy of a type, replacing type var a with type b */
val Substitute(val a, val b, val type)
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

/* updates a context, replacing type var t1 with type t2 */
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

/* updates a substitution, replacing type var t1 with type t2 */
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

/* composes two substitutions */
val CombineSubs(val s1, val s2)
{
  val subs = s1;
  while (s2) {
    val entry = Head(s2);
    if (!SubSub(Head(entry), Tail(entry), s1)) {
      subs = Pair(entry, subs);
    }
    s2 = Tail(s2);
  }
  return subs;
}

/* updates a node, replacing type var t1 with type t2 */
void SubNode(val t1, val t2, val node)
{
  val children;
  SetNodeType(node, Substitute(t1, t2, NodeValType(node)));
  switch (NodeType(node)) {
  case nilNode:
  case idNode:
  case strNode:
  case symNode:
  case intNode:
    break;
  default:
    children = NodeChildren(node);
    while (children) {
      SubNode(t1, t2, Head(children));
      children = Tail(children);
    }
  }
}

/* updates a type by applying a substitution */
val ApplySubs(val subs, val type)
{
  while (subs) {
    val sub = Head(subs);
    subs = Tail(subs);
    type = Substitute(Head(sub), Tail(sub), type);
  }
  return type;
}

/* updates a context by applying a substitution */
void ApplySubsContext(val subs, val context)
{
  while (subs) {
    val sub = Head(subs);
    subs = Tail(subs);
    SubContext(Head(sub), Tail(sub), context);
  }
}

/* updates a node by applying a substitution */
void ApplySubsNode(val subs, val node)
{
  while (subs) {
    val entry = Head(subs);
    SubNode(Head(entry), Tail(entry), node);
    subs = Tail(subs);
  }
}

void ApplySubsNodes(val subs, val nodes)
{
  while (nodes) {
    ApplySubsNode(subs, Head(nodes));
    nodes = Tail(nodes);
  }
}

val Unify(val t1, val t2, val node)
{
  val subs = 0;
  if (IsSym(t1) && t1 == t2) return 0;
  if (IsInt(t1) || IsInt(t2)) {
    if (t1 == t2) return 0;
    if (IsInt(t1)) {
      subs = Pair(Pair(t1, t2), 0);
    } else {
      subs = Pair(Pair(t2, t1), 0);
    }
    return subs;
  }
  if (IsTuple(t1) && IsTuple(t2) && TupleGet(t1, 0) == TupleGet(t2, 0)) {
    u32 i;
    val result;
    if (TupleLength(t1) != TupleLength(t2)) {
      return TypeError("Wrong number of arguments", node);
    }
    for (i = 1; i < TupleLength(t1); i++) {
      result = Unify(TupleGet(t1, i), TupleGet(t2, i), node);
      if (IsError(result)) return result;
      t1 = ApplySubs(result, t1);
      t2 = ApplySubs(result, t2);
      subs = CombineSubs(subs, result);
    }
    return subs;
  }
  return TypeError("Type mismatch", node);
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

/*** GENERALIZATION ***/

/* returns a list of type vars found in a type that aren't found in a context */
val FindFreeVars(val type, val context, val tvars)
{
  if (IsTypeConst(type)) return tvars;
  if (IsTypeVar(type) &&
      !TypeInContext(type, context) &&
      !InList(type, tvars)) {
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

/* generalizes a type into a polytype */
val Generalize(val type, val context)
{
  return PolyType(type, FindFreeVars(type, context, 0));
}

/* instantiates a polytype */
val Instantiate(val polytype)
{
  val t = Head(polytype);
  val vars = Tail(polytype);
  while (vars) {
    t = Substitute(Head(vars), NewTypeVar(), t);
    vars = Tail(vars);
  }
  return t;
}

/*** INFERENCE ***/

/* A node's type is inferred recursively, depending on the node type. When a
node's type is inferred, it's type property is updated, and any substitution it
creates are applied to the context and node. The substitution is returned. */

val InferType(val node, val context);

val InferNodes(val nodes, val context)
{
  val sub = 0;
  val prev_nodes = 0;
  while (nodes) {
    val node = Head(nodes);
    val result = InferType(node, context);
    if (IsError(result)) return result;
    ApplySubsContext(result, context);
    ApplySubsNodes(result, prev_nodes);
    sub = CombineSubs(sub, result);
    prev_nodes = Pair(node, prev_nodes);
    nodes = Tail(nodes);
  }
  return sub;
}

val InferVar(val node, val context)
{
  val varType = TypeOfVar(NodeValue(node), context);
  if (!varType) return TypeError("Undefined variable", node);
  SetNodeType(node, Instantiate(varType));
  return 0;
}

val InferConst(val node, char *t)
{
  SetNodeType(node, ConstType(t));
  return 0;
}

val InferTuple(val node, val context)
{
  val type = TypeFunc("tuple", NodeLength(node));
  val items = NodeChildren(node);
  val result = InferNodes(items, context);
  u32 i;
  if (IsError(result)) return result;
  for (i = 0; i < TupleLength(type) - 1; i++) {
    TupleSet(type, i+1, NodeValType(Head(items)));
    items = Tail(items);
  }
  SetNodeType(node, type);
  return result;
}

val InferDo(val node, val context)
{
  val stmts = DoNodeStmts(node);
  val stmt, result, prev_stmts = 0;
  while (stmts) {
    stmt = Head(stmts);
    if (NodeType(stmt) == defNode) {
      val var = NodeValue(DefNodeVar(stmt));
      context = Pair(Pair(var, PolyType(NewTypeVar(), 0)), context);
    }
    result = InferType(stmt, context);
    if (IsError(result)) return result;
    if (NodeType(stmt) == letNode) {
      val var = NodeValue(LetNodeVar(stmt));
      val stmt_type = NodeValType(stmt);
      context = Pair(Pair(var, Generalize(stmt_type, context)), context);
    }
    if (NodeType(stmt) == defNode) {
      SetTail(Head(context), Generalize(NodeValType(stmt), context));
      PrintContext(context);
    }
    ApplySubsContext(result, context);
    ApplySubsNodes(result, prev_stmts);
    prev_stmts = Pair(stmt, prev_stmts);
    stmts = Tail(stmts);
  }
  SetNodeType(node, NodeValType(stmt));
  return 0;
}

val InferLet(val node, val context)
{
  val var = LetNodeVar(node);
  val expr = LetNodeExpr(node);
  val result = InferType(expr, context);
  if (IsError(result)) return result;
  SetNodeType(var, NodeValType(expr));
  SetNodeType(node, NodeValType(expr));
  return 0;
}

val InferIf(val node, val context)
{
  val cons = IfNodeCons(node);
  val alt = IfNodeAlt(node);
  val result1, result2;
  result1 = InferNodes(NodeChildren(node), context);
  if (IsError(result1)) return result1;
  SetNodeType(node, NodeValType(cons));
  result2 = Unify(NodeValType(cons), NodeValType(alt), context);
  if (IsError(result2)) return result2;
  ApplySubsContext(result2, context);
  ApplySubsNode(result2, node);
  return CombineSubs(result1, result2);
}

val InferLambda(val node, val context)
{
  val params = LambdaNodeParams(node);
  val body = LambdaNodeBody(node);
  val lambda_type = TypeFunc("fn", ListLength(params)+1);
  val result;
  i32 i = 0;
  while (params) {
    val var = NodeValue(Head(params));
    val param_type = NewTypeVar();
    SetNodeType(Head(params), param_type);
    TupleSet(lambda_type, i+1, param_type);
    context = Pair(Pair(var, PolyType(param_type, 0)), context);
    params = Tail(params);
    i++;
  }
  result = InferType(body, context);
  if (IsError(result)) return result;
  TupleSet(lambda_type, TupleLength(lambda_type)-1, NodeValType(body));
  lambda_type = ApplySubs(result, lambda_type);
  SetNodeType(node, lambda_type);
  params = LambdaNodeParams(node);
  while (params) {
    ApplySubsNode(result, Head(params));
    params = Tail(params);
  }
  return result;
}

val OpType(i32 node_type)
{
  val type;
  switch (node_type) {
  case negNode:
  case compNode:
    type = TypeFunc("op", 2);
    TupleSet(type, 1, ConstType("int"));
    TupleSet(type, 2, ConstType("int"));
    return type;
  case lenNode:
  case notNode:
    type = TypeFunc("op", 2);
    TupleSet(type, 1, NewTypeVar());
    TupleSet(type, 2, ConstType("int"));
    return type;
  case accessNode:
    type = TypeFunc("op", 3);
    TupleSet(type, 1, NewTypeVar());
    TupleSet(type, 2, ConstType("int"));
    TupleSet(type, 3, NewTypeVar());
    return type;
  case sliceNode:
    type = TypeFunc("op", 4);
    TupleSet(type, 1, NewTypeVar());
    TupleSet(type, 2, ConstType("int"));
    TupleSet(type, 3, ConstType("int"));
    TupleSet(type, 4, NewTypeVar());
    return type;
  case mulNode:
  case divNode:
  case remNode:
  case bitandNode:
  case subNode:
  case addNode:
  case bitorNode:
  case shiftNode:
  case ltNode:
  case gtNode:
    type = TypeFunc("op", 3);
    TupleSet(type, 1, ConstType("int"));
    TupleSet(type, 2, ConstType("int"));
    TupleSet(type, 3, ConstType("int"));
    return type;
  case pairNode: {
    val a = NewTypeVar();
    val b = NewTypeVar();
    val pair_type = TypeFunc("pair", 2);
    TupleSet(pair_type, 1, a);
    TupleSet(pair_type, 2, b);
    type = TypeFunc("op", 3);
    TupleSet(type, 1, a);
    TupleSet(type, 2, b);
    TupleSet(type, 3, pair_type);
    return type;
  }
  case eqNode:
    type = TypeFunc("op", 3);
    TupleSet(type, 1, NewTypeVar());
    TupleSet(type, 2, NewTypeVar());
    TupleSet(type, 3, ConstType("int"));
    return type;
  case andNode:
  case orNode: {
    val a = NewTypeVar();
    val b = NewTypeVar();
    type = TypeFunc("op", 3);
    TupleSet(type, 1, a);
    TupleSet(type, 2, b);
    TupleSet(type, 3, Either(ConstType("int"), Either(a, b)));
    return type;
  }
  default:
    assert(false);
  }
}

val InferOp(val node, val context)
{
  val args = NodeChildren(node);
  u32 nargs = NodeLength(node);
  u32 i;
  val op_type = OpType(NodeType(node));
  val call_type = TypeFunc("op", nargs+1);
  val return_type = NewTypeVar();
  val result1, result2;
  result1 = InferNodes(args, context);
  if (IsError(result1)) return result1;
  for (i = 0; i < nargs; i++) {
    val arg = Head(args);
    TupleSet(call_type, i+1, NodeValType(arg));
    args = Tail(args);
  }
  TupleSet(call_type, nargs+1, return_type);
  SetNodeType(node, return_type);
  result2 = Unify(op_type, call_type, node);
  if (IsError(result2)) return result2;
  ApplySubsContext(result2, context);
  ApplySubsNode(result2, node);
  return CombineSubs(result1, result2);
}

val InferCall(val node, val context)
{
  val return_type = NewTypeVar();
  val fun = CallNodeFun(node);
  val args = CallNodeArgs(node);
  u32 nargs = ListLength(args);
  val call_type = TypeFunc("fn", nargs+1);
  u32 i;
  val result1, result2;

  result1 = InferNodes(NodeChildren(node), context);
  if (IsError(result1)) return result1;
  for (i = 0; i < nargs; i++) {
    val arg = Head(args);
    TupleSet(call_type, i+1, NodeValType(arg));
    args = Tail(args);
  }
  TupleSet(call_type, nargs+1, return_type);
  SetNodeType(node, return_type);
  result2 = Unify(NodeValType(fun), call_type, node);
  if (IsError(result2)) return result2;
  ApplySubsContext(result2, context);
  ApplySubsNode(result2, node);
  return CombineSubs(result1, result2);
}

val InferType(val node, val context)
{
  switch (NodeType(node)) {
  case errNode:     return node;
  case idNode:      return InferVar(node, context);
  case nilNode:     return InferConst(node, "nil");
  case intNode:     return InferConst(node, "int");
  case symNode:     return InferConst(node, "sym");
  case strNode:     return InferConst(node, "str");
  case tupleNode:   return InferTuple(node, context);
  case doNode:      return InferDo(node, context);
  case ifNode:      return InferIf(node, context);
  case lambdaNode:  return InferLambda(node, context);
  case notNode:     return InferOp(node, context);
  case lenNode:     return InferOp(node, context);
  case compNode:    return InferOp(node, context);
  case negNode:     return InferOp(node, context);
  case accessNode:  return InferOp(node, context);
  case sliceNode:   return InferOp(node, context);
  case mulNode:     return InferOp(node, context);
  case divNode:     return InferOp(node, context);
  case remNode:     return InferOp(node, context);
  case bitandNode:  return InferOp(node, context);
  case subNode:     return InferOp(node, context);
  case addNode:     return InferOp(node, context);
  case bitorNode:   return InferOp(node, context);
  case shiftNode:   return InferOp(node, context);
  case ltNode:      return InferOp(node, context);
  case gtNode:      return InferOp(node, context);
  case joinNode:    return InferOp(node, context);
  case pairNode:    return InferOp(node, context);
  case eqNode:      return InferOp(node, context);
  case andNode:     return InferOp(node, context);
  case orNode:      return InferOp(node, context);
  case callNode:    return InferCall(node, context);
  case defNode:     return InferLet(node, context);
  case letNode:     return InferLet(node, context);
  case moduleNode:  return InferType(ModNodeBody(node), context);
  default:          return TypeError("Unknown expression", node);
  }
}

val InferTypes(val node, val context)
{
  val result = InferType(node, context);
  if (IsError(result)) return result;
  return node;
}

/*** PARSE TYPES ***/

val ParseSubType(char **spec);

val ParseTypeFunc(char *name, char **spec)
{
  val params, param, t;
  u32 i;
  param = ParseSubType(spec);
  if (!param) return 0;
  params = Pair(param, 0);
  while (AdvMatch(",", spec)) {
    while (AdvMatch(" ", spec)) ;
    param = ParseSubType(spec);
    if (!param) return 0;
    params = Pair(param, params);
  }
  if (!AdvMatch(")", spec)) return 0;

  t = Tuple(ListLength(params) + 1);
  TupleSet(t, 0, SymVal(Symbol(name)));
  for (i = 1; i < TupleLength(t); i++) {
    TupleSet(t, TupleLength(t) - i, Head(params));
    params = Tail(params);
  }
  return t;
}

val ParseSubType(char **spec)
{
  if (AdvMatch("int", spec)) return SymVal(Symbol("int"));
  if (AdvMatch("str", spec)) return SymVal(Symbol("str"));
  if (AdvMatch("sym", spec)) return SymVal(Symbol("sym"));
  if (AdvMatch("nil", spec)) return SymVal(Symbol("nil"));
  if (AdvMatch("fn(", spec)) return ParseTypeFunc("fn", spec);
  if (AdvMatch("tuple(", spec)) return ParseTypeFunc("tuple", spec);
  if (AdvMatch("pair(", spec)) return ParseTypeFunc("pair", spec);

  if (IsLowercase(**spec)) {
    i32 n = **spec - 'a';
    *spec += 1;
    return IntVal(n);
  }

  return 0;
}

val ParseType(char *spec)
{
  return ParseSubType(&spec);
}
