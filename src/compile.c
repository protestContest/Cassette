#include "compile.h"
#include "parse.h"
#include "node.h"
#include "vm.h"
#include "env.h"
#include <univ/symbol.h>
#include <univ/vec.h>

typedef struct {
  val env;
  Module *mod;
} Compiler;

static val CompileExpr(val node, Compiler *c);

#define Fail(msg, node) MakeError(msg, NodeStart(node), NodeEnd(node))

static void ExtendEnv(i32 count, u32 pos, Compiler *c)
{
  PushByte(opGetEnv, pos, c->mod);
  PushByte(opTuple, pos, c->mod);
  PushInt(count, pos, c->mod);
  PushByte(opPair, pos, c->mod);
  PushByte(opSetEnv, pos, c->mod);
  c->env = Pair(Tuple(count), c->env);
}

static void PopEnv(u32 pos, Compiler *c)
{
  PushByte(opGetEnv, pos, c->mod);
  PushByte(opTail, pos, c->mod);
  PushByte(opSetEnv, pos, c->mod);
  c->env = Tail(c->env);
}

val CompileConst(val value, u32 pos, Compiler *c)
{
  if (value) {
    PushConst(value, pos, c->mod);
  } else {
    PushByte(opNil, pos, c->mod);
  }
  return 0;
}

val CompileStr(val node, Compiler *c)
{
  char *data = SymbolName(RawVal(NodeValue(node)));
  CompileConst(NodeValue(node), NodeStart(node), c);
  PushByte(opStr, NodeStart(node), c->mod);
  PushData(data, strlen(data), c->mod);
  return 0;
}

val CompileVar(val node, Compiler *c)
{
  i32 n = FindEnv(NodeValue(node), c->env);
  if (n < 0) return Fail("Undefined variable", node);
  PushByte(opLookup, NodeStart(node), c->mod);
  PushInt(n, NodeStart(node), c->mod);
  return 0;
}

val CompileTuple(val node, Compiler *c)
{
  val error;
  val items = NodeValue(node);
  i32 numItems = ListLength(items), i;
  PushByte(opTuple, NodeStart(node), c->mod);
  PushInt(numItems, NodeStart(node), c->mod);
  for (i = 0; i < numItems; i++) {
    PushByte(opDup, NodeStart(node), c->mod);
    PushConst(IntVal(i), NodeStart(node), c->mod);
    error = CompileExpr(Head(items), c);
    if (error) return error;
    PushByte(opSet, NodeStart(node), c->mod);
    items = Tail(items);
  }
  return 0;
}

val CompileLet(val node, i32 n, Compiler *c)
{
  val error;
  val var = NodeValue(NodeChild(node, 0));
  val value = NodeChild(node, 1);
  error = CompileExpr(value, c);
  if (error) return error;
  EnvSet(var, n, c->env);
  PushByte(opDefine, NodeStart(node), c->mod);
  PushInt(n, NodeStart(node), c->mod);
  return 0;
}

val CompileDef(val node, i32 n, Compiler *c)
{
  val error;
  val value = NodeChild(node, 1);
  error = CompileExpr(value, c);
  if (error) return error;
  PushByte(opDefine, NodeStart(node), c->mod);
  PushInt(n, NodeStart(node), c->mod);
  return 0;
}

val CompileDo(val node, Compiler *c)
{
  val error;
  i32 i, numAssigns = RawVal(NodeValue(NodeChild(node, 0)));
  val stmts = Tail(NodeValue(node));

  if (numAssigns > 0) ExtendEnv(numAssigns, NodeStart(node), c);

  /* pre-define all defs */
  i = numAssigns;
  while (stmts) {
    val stmt = Head(stmts);
    val var;
    stmts = Tail(stmts);
    if (NodeType(stmt) != defNode) break;
    var = NodeValue(NodeChild(stmt, 0));
    EnvSet(var, --i, c->env);
  }

  stmts = Tail(NodeValue(node));
  i = numAssigns;
  while (stmts) {
    val stmt = Head(stmts);
    stmts = Tail(stmts);
    if (NodeType(stmt) == defNode) {
      error = CompileDef(stmt, --i, c);
      if (error) return error;
      if (!stmts) CompileConst(0, NodeStart(node), c);
    } else if (NodeType(stmt) == letNode) {
      error = CompileLet(stmt, --i, c);
      if (error) return error;
      if (!stmts) CompileConst(0, NodeStart(node), c);
    } else {
      error = CompileExpr(stmt, c);
      if (error) return error;
      if (stmts) PushByte(opDrop, NodeStart(node), c->mod);
    }
  }
  if (numAssigns > 0) PopEnv(NodeStart(node), c);
  return 0;
}

val CompileIf(val node, Compiler *c)
{
  val error;
  val pred = NodeChild(node, 0);
  val cons = NodeChild(node, 1);
  val alt = NodeChild(node, 2);
  u8 *code = c->mod->code;
  u32 *srcMap = c->mod->srcMap;
  u8 *falseCode = 0;
  u32 *falseMap = 0;
  u8 *trueCode = 0;
  u32 *trueMap = 0;

  c->mod->code = trueCode;
  c->mod->srcMap = trueMap;
  error = CompileExpr(cons, c);
  if (error) return error;
  trueCode = c->mod->code;
  trueMap = c->mod->srcMap;

  c->mod->code = falseCode;
  c->mod->srcMap = falseMap;
  error = CompileExpr(alt, c);
  if (error) return error;
  PushByte(opJmp, NodeStart(node), c->mod);
  PushInt(VecCount(trueCode), NodeStart(node), c->mod);
  falseCode = c->mod->code;
  falseMap = c->mod->srcMap;

  c->mod->code = code;
  c->mod->srcMap = srcMap;
  error = CompileExpr(pred, c);
  if (error) return error;
  PushByte(opBr, NodeStart(node), c->mod);
  PushInt(VecCount(falseCode), NodeStart(node), c->mod);
  AppendCode(falseCode, falseMap, c->mod);
  AppendCode(trueCode, trueMap, c->mod);
  return 0;
}

val CompileAnd(val node, Compiler *c)
{
  val error;
  val left = NodeChild(node, 0);
  val right = NodeChild(node, 1);
  u8 *code = c->mod->code;
  u32 *srcMap = c->mod->srcMap;
  u8 *rightCode = 0;
  u32 *rightMap = 0;
  c->mod->code = rightCode;
  c->mod->srcMap = rightMap;
  PushByte(opDrop, NodeStart(node), c->mod);
  error = CompileExpr(right, c);
  if (error) return error;
  rightCode = c->mod->code;
  rightMap = c->mod->srcMap;
  c->mod->code = code;
  c->mod->srcMap = srcMap;
  error = CompileExpr(left, c);
  if (error) return error;
  PushByte(opDup, NodeStart(node), c->mod);
  PushByte(opBr, NodeStart(node), c->mod);
  PushInt(VecCount(rightCode), NodeStart(node), c->mod);
  AppendCode(rightCode, rightMap, c->mod);
  return 0;
}

val CompileOr(val node, Compiler *c)
{
  val error;
  val left = NodeChild(node, 0);
  val right = NodeChild(node, 1);
  u8 *code = c->mod->code;
  u32 *srcMap = c->mod->srcMap;
  u8 *rightCode = 0;
  u32 *rightMap = 0;
  c->mod->code = rightCode;
  c->mod->srcMap = rightMap;
  PushByte(opDrop, NodeStart(node), c->mod);
  error = CompileExpr(right, c);
  if (error) return error;
  rightCode = c->mod->code;
  rightMap = c->mod->srcMap;
  c->mod->code = code;
  c->mod->srcMap = srcMap;
  error = CompileExpr(left, c);
  if (error) return error;
  PushByte(opDup, NodeStart(node), c->mod);
  PushByte(opNot, NodeStart(node), c->mod);
  PushByte(opBr, NodeStart(node), c->mod);
  PushInt(VecCount(rightCode), NodeStart(node), c->mod);
  AppendCode(rightCode, rightMap, c->mod);
  return 0;
}

val CompileBinOp(OpCode op, val node, Compiler *c)
{
  val error;
  error = CompileExpr(NodeChild(node, 0), c);
  if (error) return error;
  error = CompileExpr(NodeChild(node, 1), c);
  if (error) return error;
  PushByte(op, NodeStart(node), c->mod);
  return 0;
}

val CompileUnaryOp(OpCode op, val node, Compiler *c)
{
  val error;
  error = CompileExpr(NodeChild(node, 0), c);
  if (error) return error;
  PushByte(op, NodeStart(node), c->mod);
  return 0;
}

#define IsTrap(node)  (NodeType(node) == idNode && RawVal(NodeValue(node)) == Symbol("trap"))

val CompileTrap(val node, Compiler *c)
{
  val error;
  val args = Tail(NodeValue(node));
  val id;
  if (!args) return Fail("Trap ID required", node);
  id = Head(args);
  args = Tail(args);
  while (args) {
    error = CompileExpr(Head(args), c);
    if (error) return error;
    args = Tail(args);
  }
  if (NodeType(id) != symNode) return Fail("Trap ID must be a symbol", node);
  PushByte(opTrap, NodeStart(node), c->mod);
  PushInt(RawVal(NodeValue(id)), NodeStart(node), c->mod);
  return 0;
}

val CompileCall(val node, Compiler *c)
{
  val error;
  val op = NodeChild(node, 0);
  val args = Tail(NodeValue(node));
  u8 *code = c->mod->code;
  u32 *srcMap = c->mod->srcMap;
  u8 *callCode = 0;
  u32 *callMap = 0;

  if (IsTrap(op)) return CompileTrap(node, c);

  c->mod->code = callCode;
  c->mod->srcMap = callMap;
  while (args) {
    error = CompileExpr(Head(args), c);
    if (error) return error;
    args = Tail(args);
  }
  error = CompileExpr(op, c);
  if (error) return error;
  PushByte(opDup, NodeStart(node), c->mod);
  PushByte(opHead, NodeStart(node), c->mod);
  PushByte(opSetEnv, NodeStart(node), c->mod);
  PushByte(opTail, NodeStart(node), c->mod);
  PushByte(opGoto, NodeStart(node), c->mod);
  callCode = c->mod->code;
  callMap = c->mod->srcMap;

  c->mod->code = code;
  c->mod->srcMap = srcMap;
  PushByte(opGetEnv, NodeStart(node), c->mod);
  PushByte(opPos, NodeStart(node), c->mod);
  PushInt(VecCount(callCode), NodeStart(node), c->mod);
  AppendCode(callCode, callMap, c->mod);
  PushByte(opSwap, NodeStart(node), c->mod);
  PushByte(opSetEnv, NodeStart(node), c->mod);
  return 0;
}

val CompileAccess(val node, Compiler *c)
{
  return CompileBinOp(opGet, node, c);
}

val CompileLambda(val node, Compiler *c)
{
  val error;
  u8 *code = c->mod->code;
  u32 *srcMap = c->mod->srcMap;
  u8 *lambdaCode = 0;
  u32 *lambdaMap = 0;
  val params = ReverseList(NodeValue(NodeChild(node, 0)), 0);
  val body = NodeChild(node, 1);
  i32 i, lambdaStart, numParams = ListLength(params);

  c->mod->code = lambdaCode;
  c->mod->srcMap = lambdaMap;
  if (numParams > 0) {
    ExtendEnv(numParams, NodeStart(node), c);
    for (i = 0; i < numParams; i++) {
      PushByte(opDefine, NodeStart(node), c->mod);
      PushInt(numParams-i-1, NodeStart(node), c->mod);
      EnvSet(NodeValue(Head(params)), numParams-i-1, c->env);
      params = Tail(params);
    }
  }
  error = CompileExpr(body, c);
  if (error) {
    FreeVec(code);
    return error;
  }
  if (numParams > 0) c->env = Tail(c->env);
  PushByte(opSwap, NodeStart(node), c->mod);
  PushByte(opGoto, NodeStart(node), c->mod);
  lambdaCode = c->mod->code;
  lambdaMap = c->mod->srcMap;

  c->mod->code = code;
  c->mod->srcMap = srcMap;
  PushByte(opJmp, NodeStart(node), c->mod);
  PushInt(VecCount(lambdaCode), NodeStart(node), c->mod);
  lambdaStart = VecCount(c->mod->code);
  AppendCode(lambdaCode, lambdaMap, c->mod);
  PushByte(opPos, NodeStart(node), c->mod);
  PushLabel(lambdaStart, NodeStart(node), c->mod);
  PushByte(opGetEnv, NodeStart(node), c->mod);
  PushByte(opPair, NodeStart(node), c->mod);
  return 0;
}

val CompileSlice(val node, Compiler *c)
{
  val error;
  error = CompileExpr(NodeChild(node, 0), c);
  if (error) return error;
  error = CompileExpr(NodeChild(node, 1), c);
  if (error) return error;
  error = CompileExpr(NodeChild(node, 2), c);
  if (error) return error;
  PushByte(opSlice, NodeStart(node), c->mod);
  return 0;
}

val CompileExpr(val node, Compiler *c)
{
  switch (NodeType(node)) {
  case nilNode:     return CompileConst(nil, NodeStart(node), c);
  case intNode:     return CompileConst(NodeValue(node), NodeStart(node), c);
  case idNode:      return CompileVar(node, c);
  case symNode:     return CompileConst(NodeValue(node), NodeStart(node), c);
  case strNode:     return CompileStr(node, c);
  case pairNode:    return CompileBinOp(opPair, node, c);
  case joinNode:    return CompileBinOp(opJoin, node, c);
  case sliceNode:   return CompileSlice(node, c);
  case tupleNode:   return CompileTuple(node, c);
  case eqNode:      return CompileBinOp(opEq, node, c);
  case ltNode:      return CompileBinOp(opLt, node, c);
  case gtNode:      return CompileBinOp(opGt, node, c);
  case shiftNode:   return CompileBinOp(opShift, node, c);
  case addNode:     return CompileBinOp(opAdd, node, c);
  case subNode:     return CompileBinOp(opSub, node, c);
  case bitorNode:   return CompileBinOp(opOr, node, c);
  case mulNode:     return CompileBinOp(opMul, node, c);
  case divNode:     return CompileBinOp(opDiv, node, c);
  case remNode:     return CompileBinOp(opRem, node, c);
  case bitandNode:  return CompileBinOp(opAnd, node, c);
  case negNode:     return CompileUnaryOp(opNeg, node, c);
  case notNode:     return CompileUnaryOp(opNot, node, c);
  case compNode:    return CompileUnaryOp(opComp, node, c);
  case lenNode:     return CompileUnaryOp(opLen, node, c);
  case accessNode:  return CompileAccess(node, c);
  case doNode:      return CompileDo(node, c);
  case ifNode:      return CompileIf(node, c);
  case andNode:     return CompileAnd(node, c);
  case orNode:      return CompileOr(node, c);
  case callNode:    return CompileCall(node, c);
  case lambdaNode:  return CompileLambda(node, c);
  default:          return Fail("Unexpected expression", node);
  }
}

val Compile(i32 ast, val env, Module *mod)
{
  Compiler c;
  c.env = env;
  c.mod = mod;
  if (NodeType(ast) == moduleNode) {
    return CompileExpr(ModNodeBody(ast), &c);
  } else {
    return CompileExpr(ast, &c);
  }
}
