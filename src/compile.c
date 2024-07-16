#include "compile.h"
#include "env.h"
#include "error.h"
#include "primitives.h"
#include <univ/symbol.h>

#define Fail(msg, node) \
  MakeError(Binary(msg), Pair(NodeStart(node), NodeEnd(node)))

#define linkNext    0
#define linkReturn  SymVal(Symbol("return"))

val CompileExpr(val node, val env, val linkage);

val CompileConst(val value, val linkage);
val CompileStr(val node, val linkage);
val CompileTuple(val node, val env, val linkage);
val CompileVar(val node, val env, val linkage);
val CompileLambda(val node, val env, val linkage);
val CompileIf(val node, val env, val linkage);
val CompileDo(val node, val env, val linkage);
val CompileCall(val node, val env, val linkage);
val CompileUnaryOp(char *op, val node, val env, val linkage);
val CompileBinOp(char *op, val node, val env, val linkage);
val CompilePair(val node, val env, val linkage);
val CompileSlice(val node, val env, val linkage);
val CompileLinkage(val linkage, val code);
val CompileAnd(val node, val env, val linkage);
val CompileOr(val node, val env, val linkage);

val NewLabel(void)
{
  static i32 nextLabel = 0;
  return IntVal(nextLabel++);
}

val PrimitiveEnv(void)
{
  u32 i, num_primitives = NumPrimitives();
  PrimDef *primitives = Primitives();
  val frame = Tuple(num_primitives);

  for (i = 0; i < num_primitives; i++) {
    TupleSet(frame, i, SymVal(Symbol(primitives[i].name)));
  }
  return Pair(frame, 0);
}

val Compile(val node)
{
  val env = PrimitiveEnv();
  if (NodeType(node) == moduleNode) {
    return CompileExpr(ModNodeBody(node), env, linkNext);
  } else {
    return CompileExpr(node, env, linkNext);
  }
}

val CompileExpr(val node, val env, val linkage)
{
  switch (NodeType(node)) {
  case nilNode:     return CompileConst(0, linkage);
  case intNode:     return CompileConst(NodeValue(node), linkage);
  case symNode:     return CompileConst(NodeValue(node), linkage);
  case strNode:     return CompileStr(node, linkage);
  case tupleNode:   return CompileTuple(node, env, linkage);
  case idNode:      return CompileVar(node, env, linkage);
  case ifNode:      return CompileIf(node, env, linkage);
  case lambdaNode:  return CompileLambda(node, env, linkage);
  case doNode:      return CompileDo(node, env, linkage);
  case callNode:    return CompileCall(node, env, linkage);
  case eqNode:      return CompileBinOp("eq", node, env, linkage);
  case ltNode:      return CompileBinOp("lt", node, env, linkage);
  case gtNode:      return CompileBinOp("gt", node, env, linkage);
  case shiftNode:   return CompileBinOp("shift", node, env, linkage);
  case addNode:     return CompileBinOp("add", node, env, linkage);
  case subNode:     return CompileBinOp("sub", node, env, linkage);
  case bitorNode:   return CompileBinOp("or", node, env, linkage);
  case mulNode:     return CompileBinOp("mul", node, env, linkage);
  case divNode:     return CompileBinOp("div", node, env, linkage);
  case remNode:     return CompileBinOp("rem", node, env, linkage);
  case bitandNode:  return CompileBinOp("and", node, env, linkage);
  case joinNode:    return CompileBinOp("join", node, env, linkage);
  case accessNode:  return CompileBinOp("get", node, env, linkage);
  case pairNode:    return CompilePair(node, env, linkage);
  case negNode:     return CompileUnaryOp("neg", node, env, linkage);
  case notNode:     return CompileUnaryOp("not", node, env, linkage);
  case compNode:    return CompileUnaryOp("comp", node, env, linkage);
  case lenNode:     return CompileUnaryOp("len", node, env, linkage);
  case sliceNode:   return CompileSlice(node, env, linkage);
  case andNode:     return CompileAnd(node, env, linkage);
  case orNode:      return CompileOr(node, env, linkage);
  default:          return Fail("Unexpected expression", node);
  }
}

val CompileConst(val value, val linkage)
{
  val chunk;
  if (value) {
    chunk = MakeChunk(0, 0, Pair(Op("const"), Pair(value, 0)));
  } else {
    chunk = MakeChunk(0, 0, Pair(Op("nil"), 0));
  }
  return CompileLinkage(linkage, chunk);
}

val CompileStr(val node, val linkage)
{
  return
    CompileLinkage(linkage,
      MakeChunk(0, 0,
        Pair(Op("const"),
        Pair(NodeValue(node),
        Pair(Op("str"), 0)))));
}

val CompileTuple(val node, val env, val linkage)
{
  val items = NodeChildren(node);
  u32 num_items = NodeLength(node);
  val items_chunk = EmptyChunk();
  u32 i;

  for (i = 0; i < num_items; i++) {
    val result = CompileExpr(Head(items), env, linkNext);
    if (IsError(result)) return result;
    items_chunk =
      AppendChunk(
        MakeChunk(0, 0, Pair(Op("const"), Pair(IntVal(i), 0))),
        AppendCode(Preserving(regEnv, items_chunk, result),
        Pair(Op("set"), 0)));
    items = Tail(items);
  }

  return
    AppendChunk(
      MakeChunk(0, 0, Pair(Op("tuple"), Pair(IntVal(num_items), 0))),
      items_chunk);
}

val CompileVar(val node, val env, val linkage)
{
  val code;
  i32 n = FindEnv(NodeValue(node), env);
  if (n < 0) return Fail("Undefined variable", node);
  code = Pair(Op("lookup"), Pair(IntVal(n), 0));
  return CompileLinkage(linkage, MakeChunk(regEnv, 0, code));
}

val CompileIf(val node, val env, val linkage)
{
  val pred, cons, alt;
  val true_label = NewLabel();
  val false_label = NewLabel();
  val after_label = NewLabel();
  val alt_linkage = linkage == linkNext ? after_label : linkage;
  pred = CompileExpr(IfNodePred(node), env, linkNext);
  if (IsError(pred)) return pred;
  cons = CompileExpr(IfNodeCons(node), env, linkage);
  if (IsError(cons)) return cons;
  alt = CompileExpr(IfNodeAlt(node), env, alt_linkage);
  if (IsError(alt)) return alt;
  pred = AppendCode(pred, Pair(Op("branch"), Pair(true_label, 0)));
  return AppendChunk(
    pred,
    AppendCode(
      ParallelChunks(
        LabelChunk(false_label, alt),
        LabelChunk(true_label, cons)),
      Pair(Op("label"), Pair(after_label, 0))));
}

val CompileExtendEnv(u32 num_assigns)
{
  return
    Pair(Op("getEnv"),
    Pair(Op("tuple"),
    Pair(IntVal(num_assigns),
    Pair(Op("pair"),
    Pair(Op("setEnv"), 0)))));
}

val CompilePopEnv(void)
{
  return
    Pair(Op("getEnv"),
    Pair(Op("tail"),
    Pair(Op("setEnv"), 0)));
}

val CompileAssign(val node, i32 n, val env)
{
  val result, assign_code;
  val var = NodeValue(LetNodeVar(node));
  val value = LetNodeExpr(node);
  result = CompileExpr(value, env, linkNext);
  if (IsError(result)) return result;
  assign_code = Pair(Op("define"), Pair(IntVal(n), 0));
  EnvSet(var, n, env);
  return Preserving(regEnv, result, MakeChunk(regEnv, 0, assign_code));
}

val CompileStmts(val stmts, u32 num_assigns, val env, val linkage)
{
  val stmt = Head(stmts);
  val rest = Tail(stmts);
  val result;

  if (NodeType(stmt) == defNode || NodeType(stmt) == letNode) {
    result = CompileAssign(stmt, --num_assigns, env);
    if (IsError(result)) return result;
    assert(rest);
  } else {
    val stmt_linkage = rest ? linkNext : linkage;
    result = CompileExpr(stmt, env, stmt_linkage);
    if (IsError(result)) return result;
    if (rest) result = AppendCode(result, Pair(Op("drop"), 0));
  }

  if (rest) {
    return Preserving(regEnv,
      result,
      CompileStmts(rest, num_assigns, env, linkage));
  } else {
    return result;
  }
}

val CompileDo(val node, val env, val linkage)
{
  val result;
  u32 i;
  u32 num_assigns = DoNodeNumAssigns(node);
  val stmts = DoNodeStmts(node);

  if (num_assigns == 0) return CompileStmts(stmts, 0, env, linkage);

  env = ExtendEnv(num_assigns, env);

  /* pre-define all def statements */
  i = num_assigns;
  while (stmts && NodeType(Head(stmts)) == defNode) {
    val stmt = Head(stmts);
    val var = NodeValue(DefNodeVar(stmt));
    EnvSet(var, --i, env);
    stmts = Tail(stmts);
  }

  result = CompileStmts(DoNodeStmts(node), num_assigns, env, linkage);
  if (IsError(result)) return result;

  return
    AppendChunk(
      MakeChunk(regEnv, regEnv, CompileExtendEnv(num_assigns)),
      result);
}

val CompileMakeLambda(val label)
{
  return MakeChunk(regEnv, 0,
    Pair(Op("pos"),
    Pair(label,
    Pair(Op("getEnv"),
    Pair(Op("pair"), 0)))));
}

val CompileLambdaBody(val node, val label, val env)
{
  val params = LambdaNodeParams(node);
  u32 num_params = ListLength(params);
  val body = LambdaNodeBody(node);
  val params_code = 0;
  val result;

  if (num_params > 0) {
    u32 i;
    env = ExtendEnv(num_params, env);
    for (i = 0; i < num_params; i++) {
      val param = NodeValue(Head(params));
      EnvSet(param, i, env);
      params_code = Pair(params_code, Pair(Op("define"), Pair(IntVal(num_params - i - 1), 0)));
      params = Tail(params);
    }

    params_code = Pair(CompileExtendEnv(num_params), Pair(params_code, 0));
  }

  result = CompileExpr(body, env, linkReturn);
  if (IsError(result)) return result;

  if (num_params > 0) {
    return AppendChunk(MakeChunk(regEnv, regEnv, params_code), result);
  } else {
    return result;
  }
}

val CompileLambda(val node, val env, val linkage)
{
  val body_label = NewLabel();
  val after_label = NewLabel();
  val lambda_linkage = linkage == linkNext ? after_label : linkage;
  val body = CompileLambdaBody(node, body_label, env);
  if (IsError(body)) return body;
  return
    AppendCode(
      TackOnChunk(
        CompileLinkage(lambda_linkage, CompileMakeLambda(body_label)),
        LabelChunk(body_label, body)),
      Pair(Op("label"), Pair(after_label, 0)));
}

bool IsPrimitive(val node, val env)
{
  if (NodeType(node) != idNode) return false;
  return FindFrame(NodeValue(node), env) == 0;
}

val CompileCallCode(void)
{
  return
    MakeChunk(0, regEnv,
      Pair(Op("dup"),
      Pair(Op("head"),
      Pair(Op("setEnv"),
      Pair(Op("tail"),
      Pair(Op("goto"), 0))))));
}

val CompileCall(val node, val env, val linkage)
{
  val after_label = NewLabel();
  val op = CallNodeFun(node);
  val args = ReverseList(CallNodeArgs(node), 0);
  val args_chunk = EmptyChunk();
  val result;

  while (args) {
    val arg = Head(args);
    args = Tail(args);
    result = CompileExpr(arg, env, linkNext);
    if (IsError(result)) return result;
    args_chunk = Preserving(regEnv, result, args_chunk);
  }

  if (IsPrimitive(op, env)) {
    return AppendCode(args_chunk, Pair(Op("trap"), Pair(NodeValue(op), 0)));
  }

  result = CompileExpr(op, env, linkNext);
  if (IsError(result)) return result;

  result = AppendChunk(
    Preserving(regEnv, args_chunk, result),
    CompileCallCode());

  if (linkage == linkNext) linkage = after_label;
  if (linkage == linkReturn) return result;

  return
    AppendCode(
      AppendChunk(
        MakeChunk(0, 0, Pair(Op("pos"), Pair(linkage, 0))),
        result),
      Pair(Op("label"), Pair(linkage, 0)));

}

val CompileUnaryOp(char *op, val node, val env, val linkage)
{
  val result = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(result)) return result;
  return CompileLinkage(linkage, AppendCode(result, Pair(Op(op), 0)));
}

val CompileBinOp(char *op, val node, val env, val linkage)
{
  val left, right;
  left = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, linkNext);
  if (IsError(right)) return right;

  return
    CompileLinkage(linkage,
      AppendCode(
        Preserving(regEnv, left, right),
        Pair(Op(op), 0)));
}

val CompilePair(val node, val env, val linkage)
{
  val head, tail;
  head = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(head)) return head;
  tail = CompileExpr(NodeChild(node, 1), env, linkNext);
  if (IsError(tail)) return tail;

  return
    CompileLinkage(linkage,
      AppendCode(
        Preserving(regEnv, tail, head),
        Pair(Op("pair"), 0)));
}

val CompileSlice(val node, val env, val linkage)
{
  val a, b, c;
  a = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(a)) return a;
  b = CompileExpr(NodeChild(node, 1), env, linkNext);
  if (IsError(b)) return b;
  c = CompileExpr(NodeChild(node, 2), env, linkNext);
  if (IsError(c)) return c;

  return CompileLinkage(linkage,
    AppendCode(
      Preserving(regEnv, a, Preserving(regEnv, b, c)),
      Pair(Op("slice"), 0)));
}

val CompileLinkage(val linkage, val chunk)
{
  if (linkage == linkNext) return chunk;
  if (linkage == linkReturn) {
    return AppendCode(chunk, Pair(Op("swap"), Pair(Op("goto"), 0)));
  }

  return
    AppendChunk(
      MakeChunk(0, 0, Pair(Op("pos"), Pair(linkage, 0))),
      AppendCode(chunk, Pair(Op("goto"),  0)));
}

/*
  <expr1>
  dup
  not
  branch :after
  drop
  <expr2>
after:
*/

val CompileAnd(val node, val env, val linkage)
{
  val left, right, after_label, left_linkage;
  left = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, linkNext);
  if (IsError(right)) return right;

  after_label = NewLabel();

  if (linkage == linkNext || linkage == linkReturn) {
    left_linkage = after_label;
  } else {
    left_linkage = linkage;
  }

  left =
    AppendCode(left,
      Pair(Op("dup"),
      Pair(Op("not"),
      Pair(Op("branch"), Pair(left_linkage,
      Pair(Op("drop"), 0))))));

  right = AppendCode(right, Pair(Op("label"), Pair(after_label, 0)));

  return CompileLinkage(linkage, Preserving(regEnv, left, right));
}

/*
  <expr1>
  dup
  branch :after
  drop
  <expr2>
after:
*/

val CompileOr(val node, val env, val linkage)
{
  val left, right, after_label, left_linkage;
  left = CompileExpr(NodeChild(node, 0), env, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, linkNext);
  if (IsError(right)) return right;

  after_label = NewLabel();

  if (linkage == linkNext || linkage == linkReturn) {
    left_linkage = after_label;
  } else {
    left_linkage = linkage;
  }

  left =
    AppendCode(left,
      Pair(Op("dup"),
      Pair(Op("branch"), Pair(left_linkage,
      Pair(Op("drop"), 0)))));

  right = AppendCode(right, Pair(Op("label"), Pair(after_label, 0)));

  return CompileLinkage(linkage, Preserving(regEnv, left, right));
}
