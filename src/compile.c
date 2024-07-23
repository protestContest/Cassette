#include "compile.h"
#include "error.h"
#include "primitives.h"
#include <univ/symbol.h>

#define Fail(msg, node) \
  MakeError(Binary(msg), 0, NodeStart(node), NodeEnd(node))

#define linkNext    0
#define linkReturn  SymVal(Symbol("return"))

Chunk CompileExpr(Node node, Env env, val modules, val linkage);
Chunk CompileModule(Node node, Env env, val modules, val linkage);
Chunk CompileImports(Node node, u32 num_imports, Env env);
Chunk CompileExports(Node node, Env env, val modules, val linkage);

Chunk CompileConst(val value, val linkage);
Chunk CompileStr(Node node, val linkage);
Chunk CompileVar(Node node, Env env, val modules, val linkage);
Chunk CompileTuple(Node node, Env env, val modules, val linkage);
Chunk CompileUnaryOp(OpCode op, Node node, Env env, val modules, val linkage);
Chunk CompileBinOp(OpCode op, Node node, Env env, val modules, val linkage);
Chunk CompilePair(Node node, Env env, val modules, val linkage);
Chunk CompileSlice(Node node, Env env, val modules, val linkage);

Chunk CompileAnd(Node node, Env env, val modules, val linkage);
Chunk CompileOr(Node node, Env env, val modules, val linkage);
Chunk CompileIf(Node node, Env env, val modules, val linkage);

Chunk CompileDo(Node node, Env env, val modules, val linkage);
Chunk CompileStmts(val stmts, u32 num_assigns, Env env, val modules, val linkage);

Chunk CompileLambda(Node node, Env env, val modules, val linkage);
Chunk CompileCall(Node node, Env env, val modules, val linkage);

Chunk CompileQualify(Node node, Env env, val modules, val linkage);

Chunk CompileLinkage(val linkage, val code);
Chunk CompileExtendEnv(u32 num_assigns);
Chunk CompilePopEnv(void);

val NewLabel(void)
{
  static i32 nextLabel = 0;
  return IntVal(nextLabel++);
}

val PrimitiveEnv(void)
{
  u32 i, num_primitives = NumPrimitives();
  PrimDef *primitives = Primitives();
  Env env = ExtendEnv(num_primitives, 0);

  for (i = 0; i < num_primitives; i++) {
    EnvSet(SymVal(Symbol(primitives[i].name)), i, env);
  }
  return env;
}

val Compile(Node node, Env env, val modules)
{
  return CompileExpr(node, env, modules, linkNext);
}

val CompileExpr(Node node, Env env, val modules, val linkage)
{
  switch (NodeType(node)) {
  case nilNode:     return CompileConst(0, linkage);
  case intNode:     return CompileConst(NodeValue(node), linkage);
  case symNode:     return CompileConst(NodeValue(node), linkage);
  case strNode:     return CompileStr(node, linkage);
  case tupleNode:   return CompileTuple(node, env, modules, linkage);
  case idNode:      return CompileVar(node, env, modules, linkage);
  case ifNode:      return CompileIf(node, env, modules, linkage);
  case lambdaNode:  return CompileLambda(node, env, modules, linkage);
  case doNode:      return CompileDo(node, env, modules, linkage);
  case callNode:    return CompileCall(node, env, modules, linkage);
  case eqNode:      return CompileBinOp(opEq, node, env, modules, linkage);
  case ltNode:      return CompileBinOp(opLt, node, env, modules, linkage);
  case gtNode:      return CompileBinOp(opGt, node, env, modules, linkage);
  case shiftNode:   return CompileBinOp(opShift, node, env, modules, linkage);
  case addNode:     return CompileBinOp(opAdd, node, env, modules, linkage);
  case subNode:     return CompileBinOp(opSub, node, env, modules, linkage);
  case bitorNode:   return CompileBinOp(opOr, node, env, modules, linkage);
  case mulNode:     return CompileBinOp(opMul, node, env, modules, linkage);
  case divNode:     return CompileBinOp(opDiv, node, env, modules, linkage);
  case remNode:     return CompileBinOp(opRem, node, env, modules, linkage);
  case bitandNode:  return CompileBinOp(opAnd, node, env, modules, linkage);
  case joinNode:    return CompileBinOp(opJoin, node, env, modules, linkage);
  case accessNode:  return CompileBinOp(opGet, node, env, modules, linkage);
  case qualifyNode: return CompileQualify(node, env, modules, linkage);
  case pairNode:    return CompilePair(node, env, modules, linkage);
  case negNode:     return CompileUnaryOp(opNeg, node, env, modules, linkage);
  case notNode:     return CompileUnaryOp(opNot, node, env, modules, linkage);
  case compNode:    return CompileUnaryOp(opComp, node, env, modules, linkage);
  case lenNode:     return CompileUnaryOp(opLen, node, env, modules, linkage);
  case sliceNode:   return CompileSlice(node, env, modules, linkage);
  case andNode:     return CompileAnd(node, env, modules, linkage);
  case orNode:      return CompileOr(node, env, modules, linkage);
  case moduleNode:  return CompileModule(node, env, modules, linkage);
  default:          return Fail("Unexpected expression", node);
  }
}

Chunk CompileModule(Node node, Env env, val modules, val linkage)
{
  /*
  - compile imports:
    - extend env for aliased imports
    - for each import:
      - look up module name in env
      - redefine as alias in env
  - compile body:
    - extend env for any assigns
    - pre-define each def node variable
    - compile each statement, preserving regEnv
    - drop each statement result
  - compile exports:
    - make a tuple to hold exports
    - for each export:
      - lookup export in env (should have been defined)
      - set exported value in exports
    - pop assigns env
    - pop aliased imports env
    - define exports in module env
  */

  val import_chunk, body_chunk;
  Node imports = ModNodeImports(node);
  Node body = ModNodeBody(node);
  u32 num_imports = NodeLength(imports);
  u32 num_assigns = DoNodeNumAssigns(body);
  val stmts = DoNodeStmts(body);

  if (num_imports > 0) {
    env = ExtendEnv(num_imports, env);
    import_chunk = CompileImports(imports, num_imports, env);
    if (IsError(import_chunk)) return import_chunk;
    import_chunk = AppendChunk(CompileExtendEnv(num_imports), import_chunk);
  } else {
    import_chunk = EmptyChunk();
  }

  if (num_assigns > 0) {
    u32 i = num_assigns;
    env = ExtendEnv(num_assigns, env);
    while (stmts && NodeType(Head(stmts)) == defNode) {
      val stmt = Head(stmts);
      val var = NodeValue(DefNodeVar(stmt));
      EnvSet(var, --i, env);
      stmts = Tail(stmts);
    }
  }

  body_chunk = CompileStmts(DoNodeStmts(body), num_assigns, env, modules, linkage);
  if (IsError(body_chunk)) return body_chunk;
  body_chunk = Preserving(regEnv,
    body_chunk,
    MakeChunk(regEnv, 0, Pair(Op(opDrop), 0)));

  if (num_assigns > 0) {
    body_chunk = AppendChunk(CompileExtendEnv(num_assigns), body_chunk);
  }

  return
    AppendChunk(import_chunk,
    AppendChunk(body_chunk,
      CompileExports(node, env, modules, linkNext)));
}

Chunk CompileImports(Node node, u32 num_imports, Env env)
{
  Node imports = NodeValue(node);
  Chunk chunk = EmptyChunk();
  u32 importnum = 0;

  while (imports) {
    val import = Head(imports);
    val mod = ImportNodeName(import);
    val alias = ImportNodeAlias(import);
    i32 n = FindEnv(mod, env);
    if (n < 0) return Fail("Module not found", node);
    chunk = AppendChunk(chunk,
      MakeChunk(regEnv, 0,
        Pair(Op(opLookup),
        Pair(IntVal(n),
        Pair(Op(opDefine),
        Pair(IntVal(importnum), 0))))));
    EnvSet(alias, importnum, env);
    importnum++;
    imports = Tail(imports);
  }

  return chunk;
}

Chunk CompileExports(Node node, Env env, val modules, val linkage)
{
  val chunk;
  val exports = NodeValue(ModNodeExports(node));
  u32 num_exports = ListLength(exports);
  u32 num_imports = NodeLength(ModNodeImports(node));
  u32 num_assigns = DoNodeNumAssigns(ModNodeBody(node));
  u32 i;

  chunk = MakeChunk(0, 0,
    Pair(Op(opTuple),
    Pair(IntVal(num_exports), 0)));

  i = 0;
  while (exports) {
    val export = Head(exports);
    chunk =
      AppendChunk(chunk,
      AppendChunk(MakeChunk(0, 0, Pair(Op(opConst), Pair(IntVal(i), 0))),
      AppendChunk(CompileVar(export, env, modules, linkNext),
        MakeChunk(0, 0, Pair(Op(opSet), 0)))));
    exports = Tail(exports);
    i++;
  }

  if (num_assigns > 0) chunk = AppendChunk(chunk, CompilePopEnv());
  if (num_imports > 0) chunk = AppendChunk(chunk, CompilePopEnv());

  return chunk;
}

Chunk CompileDefModule(u32 modnum)
{
  return MakeChunk(regEnv, 0, Pair(Op(opDefine), Pair(IntVal(modnum), 0)));
}

val CompileConst(val value, val linkage)
{
  val chunk;
  if (value) {
    chunk = MakeChunk(0, 0, Pair(Op(opConst), Pair(value, 0)));
  } else {
    chunk = MakeChunk(0, 0, Pair(Op(opNil), 0));
  }
  return CompileLinkage(linkage, chunk);
}

val CompileStr(Node node, val linkage)
{
  return
    CompileLinkage(linkage,
      MakeChunk(0, 0,
        Pair(Op(opConst),
        Pair(NodeValue(node),
        Pair(Op(opStr), 0)))));
}

val CompileTuple(Node node, Env env, val modules, val linkage)
{
  val items = NodeChildren(node);
  u32 num_items = NodeLength(node);
  val items_chunk = EmptyChunk();
  u32 i;

  for (i = 0; i < num_items; i++) {
    val result = CompileExpr(Head(items), env, modules, linkNext);
    if (IsError(result)) return result;
    items_chunk =
      AppendChunk(
        MakeChunk(0, 0, Pair(Op(opConst), Pair(IntVal(i), 0))),
        AppendCode(Preserving(regEnv, items_chunk, result),
        Pair(Op(opSet), 0)));
    items = Tail(items);
  }

  return
    AppendChunk(
      MakeChunk(0, 0, Pair(Op(opTuple), Pair(IntVal(num_items), 0))),
      items_chunk);
}

val CompileVar(Node node, Env env, val modules, val linkage)
{
  val code;
  i32 n = FindEnv(NodeValue(node), env);
  if (n < 0) return Fail("Undefined variable", node);
  code = Pair(Op(opLookup), Pair(IntVal(n), 0));
  return CompileLinkage(linkage, MakeChunk(regEnv, 0, code));
}

val CompileIf(Node node, Env env, val modules, val linkage)
{
  val pred, cons, alt;
  val true_label = NewLabel();
  val after_label = NewLabel();
  val alt_linkage = linkage == linkNext ? after_label : linkage;
  pred = CompileExpr(IfNodePred(node), env, modules, linkNext);
  if (IsError(pred)) return pred;
  cons = CompileExpr(IfNodeCons(node), env, modules, linkage);
  if (IsError(cons)) return cons;
  alt = CompileExpr(IfNodeAlt(node), env, modules, alt_linkage);
  if (IsError(alt)) return alt;
  pred = AppendCode(pred, Pair(Op(opBranch), Pair(true_label, 0)));
  return AppendChunk(
    pred,
    AppendCode(
      ParallelChunks(
        alt,
        LabelChunk(true_label, cons)),
      Pair(Op(opLabel), Pair(after_label, 0))));
}

Chunk CompileExtendEnv(u32 num_assigns)
{
  return MakeChunk(regEnv, regEnv,
    Pair(Op(opGetEnv),
    Pair(Op(opTuple),
    Pair(IntVal(num_assigns),
    Pair(Op(opPair),
    Pair(Op(opSetEnv), 0))))));
}

Chunk CompilePopEnv(void)
{
  return
    MakeChunk(regEnv, regEnv,
      Pair(Op(opGetEnv),
      Pair(Op(opTail),
      Pair(Op(opSetEnv), 0))));
}

val CompileAssign(Node node, i32 n, Env env, val modules)
{
  val result, assign_code;
  val var = NodeValue(LetNodeVar(node));
  val value = LetNodeExpr(node);
  result = CompileExpr(value, env, modules, linkNext);
  if (IsError(result)) return result;
  assign_code = Pair(Op(opDefine), Pair(IntVal(n), 0));
  EnvSet(var, n, env);
  return Preserving(regEnv, result, MakeChunk(regEnv, 0, assign_code));
}

val CompileStmts(val stmts, u32 num_assigns, Env env, val modules, val linkage)
{
  val stmt = Head(stmts);
  val rest = Tail(stmts);
  val result;

  if (NodeType(stmt) == defNode || NodeType(stmt) == letNode) {
    result = CompileAssign(stmt, --num_assigns, env, modules);
    if (IsError(result)) return result;
    assert(rest);
  } else {
    val stmt_linkage = rest ? linkNext : linkage;
    result = CompileExpr(stmt, env, modules, stmt_linkage);
    if (IsError(result)) return result;
    if (rest) result = AppendCode(result, Pair(Op(opDrop), 0));
  }

  if (rest) {
    return Preserving(regEnv,
      result,
      CompileStmts(rest, num_assigns, env, modules, linkage));
  } else {
    return result;
  }
}

val CompileDo(Node node, Env env, val modules, val linkage)
{
  val result;
  u32 i;
  u32 num_assigns = DoNodeNumAssigns(node);
  val stmts = DoNodeStmts(node);

  if (num_assigns == 0) return CompileStmts(stmts, 0, env, modules, linkage);

  env = ExtendEnv(num_assigns, env);

  /* pre-define all def statements */
  i = num_assigns;
  while (stmts && NodeType(Head(stmts)) == defNode) {
    val stmt = Head(stmts);
    val var = NodeValue(DefNodeVar(stmt));
    EnvSet(var, --i, env);
    stmts = Tail(stmts);
  }

  result = CompileStmts(DoNodeStmts(node), num_assigns, env, modules, linkNext);
  if (IsError(result)) return result;

  return
    CompileLinkage(linkage,
      AppendChunk(
        CompileExtendEnv(num_assigns),
        Preserving(regEnv, result,
          CompilePopEnv())));
}

val CompileMakeLambda(val label)
{
  return MakeChunk(regEnv, 0,
    Pair(Op(opPos),
    Pair(label,
    Pair(Op(opGetEnv),
    Pair(Op(opPair), 0)))));
}

val CompileLambdaBody(Node node, Env env, val modules)
{
  val params = LambdaNodeParams(node);
  u32 num_params = ListLength(params);
  val body = LambdaNodeBody(node);
  Chunk params_code = EmptyChunk();
  val result;

  if (num_params > 0) {
    u32 i;
    params_code = CompileExtendEnv(num_params);
    env = ExtendEnv(num_params, env);
    for (i = 0; i < num_params; i++) {
      val param = NodeValue(Head(params));
      EnvSet(param, i, env);
      params_code = AppendCode(params_code, Pair(Op(opDefine), Pair(IntVal(num_params - i - 1), 0)));
      params = Tail(params);
    }
  }

  result = CompileExpr(body, env, modules, linkReturn);
  if (IsError(result)) return result;

  if (num_params > 0) {
    return AppendChunk(params_code, result);
  } else {
    return result;
  }
}

val CompileLambda(Node node, Env env, val modules, val linkage)
{
  val body_label = NewLabel();
  val after_label = NewLabel();
  val lambda_linkage = linkage == linkNext ? after_label : linkage;
  val body = CompileLambdaBody(node, env, modules);
  if (IsError(body)) return body;
  return
    AppendCode(
      TackOnChunk(
        CompileLinkage(lambda_linkage, CompileMakeLambda(body_label)),
        LabelChunk(body_label, body)),
      Pair(Op(opLabel), Pair(after_label, 0)));
}

bool IsPrimitive(Node node, Env env)
{
  if (NodeType(node) != idNode) return false;
  return FindFrame(NodeValue(node), env) == 0;
}

val CompileCallCode(void)
{
  return
    MakeChunk(0, regEnv,
      Pair(Op(opDup),
      Pair(Op(opHead),
      Pair(Op(opSetEnv),
      Pair(Op(opTail),
      Pair(Op(opGoto), 0))))));
}

val CompileCall(Node node, Env env, val modules, val linkage)
{
  val after_label = NewLabel();
  val op = CallNodeFun(node);
  val args = ReverseList(CallNodeArgs(node), 0);
  val args_chunk = EmptyChunk();
  val result;

  while (args) {
    val arg = Head(args);
    args = Tail(args);
    result = CompileExpr(arg, env, modules, linkNext);
    if (IsError(result)) return result;
    args_chunk = Preserving(regEnv, result, args_chunk);
  }

  if (IsPrimitive(op, env)) {
    return AppendCode(args_chunk, Pair(Op(opTrap), Pair(NodeValue(op), 0)));
  }

  result = CompileExpr(op, env, modules, linkNext);
  if (IsError(result)) return result;

  result = AppendChunk(
    Preserving(regEnv, args_chunk, result),
    CompileCallCode());

  if (linkage == linkNext) linkage = after_label;
  if (linkage == linkReturn) return result;

  return
    AppendCode(
      AppendChunk(
        MakeChunk(0, 0, Pair(Op(opPos), Pair(linkage, 0))),
        result),
      Pair(Op(opLabel), Pair(after_label, 0)));

}

val CompileUnaryOp(OpCode op, Node node, Env env, val modules, val linkage)
{
  val result = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(result)) return result;
  return CompileLinkage(linkage, AppendCode(result, Pair(Op(op), 0)));
}

val CompileBinOp(OpCode op, Node node, Env env, val modules, val linkage)
{
  val left, right;
  left = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, modules, linkNext);
  if (IsError(right)) return right;

  return
    CompileLinkage(linkage,
      AppendCode(
        Preserving(regEnv, left, right),
        Pair(Op(op), 0)));
}

val CompileQualify(Node node, Env env, val modules, val linkage)
{
  val code, module, fn;
  Node modname = NodeChild(node, 0);
  u32 i;
  i32 n = FindEnv(NodeValue(modname), env);
  if (n < 0) return Fail("Module not imported", modname);
  code = MakeChunk(regEnv, 0, Pair(Op(opLookup), Pair(IntVal(n), 0)));

  module = TupleGet(modules, n);
  fn = NodeValue(NodeChild(node, 1));
  assert(module);
  for (i = 0; i < TupleLength(module); i++) {
    printf("m%d: %s\n", i, MemValStr(TupleGet(module, i)));
    if (TupleGet(module, i) == fn) break;
  }
  if (i == TupleLength(module)) {
    return Fail("Module not found", node);
  }
  code = AppendCode(code, Pair(Op(opConst), Pair(IntVal(i), Pair(Op(opGet), 0))));
  return CompileLinkage(linkage, code);
}

val CompilePair(Node node, Env env, val modules, val linkage)
{
  val head, tail;
  head = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(head)) return head;
  tail = CompileExpr(NodeChild(node, 1), env, modules, linkNext);
  if (IsError(tail)) return tail;

  return
    CompileLinkage(linkage,
      AppendCode(
        Preserving(regEnv, tail, head),
        Pair(Op(opPair), 0)));
}

val CompileSlice(Node node, Env env, val modules, val linkage)
{
  val a, b, c;
  a = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(a)) return a;
  b = CompileExpr(NodeChild(node, 1), env, modules, linkNext);
  if (IsError(b)) return b;
  c = CompileExpr(NodeChild(node, 2), env, modules, linkNext);
  if (IsError(c)) return c;

  return CompileLinkage(linkage,
    AppendCode(
      Preserving(regEnv, a, Preserving(regEnv, b, c)),
      Pair(Op(opSlice), 0)));
}

val CompileLinkage(val linkage, val chunk)
{
  if (linkage == linkNext) return chunk;
  if (linkage == linkReturn) {
    return AppendCode(chunk, Pair(Op(opSwap), Pair(Op(opGoto), 0)));
  }

  return
    AppendChunk(
      MakeChunk(0, 0, Pair(Op(opPos), Pair(linkage, 0))),
      AppendCode(chunk, Pair(Op(opGoto),  0)));
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

val CompileAnd(Node node, Env env, val modules, val linkage)
{
  val left, right, after_label, left_linkage;
  left = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, modules, linkNext);
  if (IsError(right)) return right;

  after_label = NewLabel();

  if (linkage == linkNext || linkage == linkReturn) {
    left_linkage = after_label;
  } else {
    left_linkage = linkage;
  }

  left =
    AppendCode(left,
      Pair(Op(opDup),
      Pair(Op(opNot),
      Pair(Op(opBranch), Pair(left_linkage,
      Pair(Op(opDrop), 0))))));

  right = AppendCode(right, Pair(Op(opLabel), Pair(after_label, 0)));

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

val CompileOr(Node node, Env env, val modules, val linkage)
{
  val left, right, after_label, left_linkage;
  left = CompileExpr(NodeChild(node, 0), env, modules, linkNext);
  if (IsError(left)) return left;
  right = CompileExpr(NodeChild(node, 1), env, modules, linkNext);
  if (IsError(right)) return right;

  after_label = NewLabel();

  if (linkage == linkNext || linkage == linkReturn) {
    left_linkage = after_label;
  } else {
    left_linkage = linkage;
  }

  left =
    AppendCode(left,
      Pair(Op(opDup),
      Pair(Op(opBranch), Pair(left_linkage,
      Pair(Op(opDrop), 0)))));

  right = AppendCode(right, Pair(Op(opLabel), Pair(after_label, 0)));

  return CompileLinkage(linkage, Preserving(regEnv, left, right));
}

void PrintModules(val modules)
{
  u32 i;
  for (i = 0; i < TupleLength(modules); i++) {
    u32 j;
    val mod = TupleGet(modules, i);
    printf("%d: ", i);
    for (j = 0; j < TupleLength(mod); j++) {
      printf("%s ", SymbolName(RawVal(TupleGet(mod, j))));
    }
    printf("\n");
  }
}
