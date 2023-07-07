#include "compile.h"
#include "parse.h"
#include "seq.h"
#include "assemble.h"
#include "ops.h"

static Seq CompileExpr(Val ast, Linkage linkage, Mem *mem);

Chunk Compile(char *source)
{
  Mem mem;
  InitMem(&mem);
  Val ast = Parse(source, &mem);

  Print("Parsed: ");
  PrintVal(ast, &mem);
  Print("\n");

  Seq code = CompileExpr(ast, LinkNext, &mem);

  Print("Compiled:\n");
  PrintSeq(code, &mem);

  return Assemble(code, &mem);
}

static Seq CompileConst(Val node, Linkage linkage, Mem *mem)
{
  return EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node, nil, mem), mem)), mem);
}

static Seq CompileVar(Val node, Linkage linkage, Mem *mem)
{
  return EndWithLinkage(linkage,
    MakeSeq(RegEnv, 0,
      Pair(OpSymbol(OpLookup),
      Pair(node, nil, mem), mem)), mem);
}

static Seq CompileSymbol(Val node, Linkage linkage, Mem *mem)
{
  node = ListAt(node, 1, mem);
  return EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node, nil, mem), mem)), mem);
}

static Seq CompileLambda(Val node, Linkage linkage, Mem *mem)
{
  Val params = ListAt(node, 1, mem);
  Seq body = CompileExpr(ListAt(node, 2, mem), LinkReturn, mem);

  Seq param_code = EmptySeq();
  while (!IsNil(params)) {
    Seq param = MakeSeq(0, RegEnv,
      Pair(OpSymbol(OpDefine),
      Pair(Head(params, mem), nil, mem), mem));
    param_code = Preserving(RegEnv | RegCont, param, param_code, mem);
    params = Tail(params, mem);
  }

  Val entry = MakeLabel(mem);
  Val after_lambda = MakeLabel(mem);

  body =
    AppendSeq(LabelSeq(entry, mem),
    AppendSeq(param_code,
    AppendSeq(body,
              LabelSeq(after_lambda, mem), mem), mem), mem);

  if (Eq(linkage, LinkNext)) linkage = after_lambda;

  Seq lambda_code =
    EndWithLinkage(linkage,
      MakeSeq(RegEnv, 0,
        Pair(OpSymbol(OpLambda),
        Pair(LabelRef(entry, mem), nil, mem), mem)), mem);

  return TackOnSeq(lambda_code, body, mem);
}

static Seq OpSeq(OpCode op, Mem *mem)
{
  return MakeSeq(0, 0, Pair(OpSymbol(op), nil, mem));
}

static Seq CompilePrefix(OpCode op, Val node, Linkage linkage, Mem *mem)
{
  Seq args = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  return Preserving(RegCont, args, EndWithLinkage(linkage, OpSeq(op, mem), mem), mem);
}

static Seq CompileInfix(Seq op_seq, Val node, Linkage linkage, Mem *mem)
{
  Seq a_code = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  Seq b_code = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);
  Seq args = Preserving(RegEnv, a_code, b_code, mem);

  return Preserving(RegCont, args, EndWithLinkage(linkage, op_seq, mem), mem);
}

static Seq CompileCall(Val node, Linkage linkage, Mem *mem)
{
  Seq args = EmptySeq();
  u32 num_args = 0;
  while (!IsNil(node)) {
    Seq arg = CompileExpr(Head(node, mem), LinkNext, mem);
    num_args++;
    args = Preserving(RegEnv, args, arg, mem);
    node = Tail(node, mem);
  }

  if (Eq(linkage, LinkReturn)) {
    return Preserving(RegCont,
      args,
      MakeSeq(RegCont, 0,
        Pair(OpSymbol(OpApply),
        Pair(IntVal(num_args), nil, mem), mem)), mem);
  } else {
    Val after_call = MakeLabel(mem);
    if (Eq(linkage, LinkNext)) linkage = after_call;

    return
      AppendSeq(args,
        MakeSeq(0, RegCont,
          Pair(OpSymbol(OpCont),
          Pair(LabelRef(linkage, mem),
          Pair(OpSymbol(OpApply),
          Pair(IntVal(num_args),
          Pair(Label(after_call, mem), nil, mem), mem), mem), mem), mem)), mem);
  }
}

static Seq CompileDo(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq stmts = EmptySeq();
  if (IsNil(node)) return stmts;

  while (!IsNil(Tail(node, mem))) {
    Seq stmt = CompileExpr(Head(node, mem), LinkNext, mem);
    stmts = AppendSeq(
      Preserving(RegEnv, stmts, stmt, mem),
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)), mem);

    node = Tail(node, mem);
  }

  Seq last_stmt = CompileExpr(Head(node, mem), linkage, mem);
  return Preserving(RegEnv | RegCont, stmts, last_stmt, mem);
}

static Seq CompileLet(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq defines = EmptySeq();

  while (!IsNil(node)) {
    Val pair = Head(node, mem);
    Val var = Head(pair, mem);

    Seq def = AppendSeq(
      CompileExpr(Tail(pair, mem), LinkNext, mem),
      MakeSeq(0, 0,
        Pair(OpSymbol(OpDefine),
        Pair(var, nil, mem), mem)), mem);

    defines = Preserving(RegEnv, defines, def, mem);
    node = Tail(node, mem);
  }

  return defines;
}

static Seq CompileDef(Val node, Linkage linkage, Mem *mem)
{
  return EmptySeq();
}

static Seq CompileIf(Val node, Linkage linkage, Mem *mem)
{
  return EmptySeq();
}

static Seq CompileCond(Val node, Linkage linkage, Mem *mem)
{
  return EmptySeq();
}

static Seq CompileLogic(Val node, Linkage linkage, Mem *mem)
{
  return EmptySeq();
}

static Seq CompileExpr(Val node, Linkage linkage, Mem *mem)
{
  if (IsNil(node))                  return EmptySeq();
  if (IsNumeric(node))              return CompileConst(node, linkage, mem);
  if (IsSym(node))                  return CompileVar(node, linkage, mem);
  if (IsTagged(node, ":", mem))     return CompileSymbol(node, linkage, mem);
  if (IsTagged(node, "->", mem))    return CompileLambda(node, linkage, mem);

  if (IsTagged(node, "do", mem))    return CompileDo(node, linkage, mem);

  if (IsTagged(node, "+", mem))     return CompileInfix(OpSeq(OpAdd, mem), node, linkage, mem);
  if (IsTagged(node, "-", mem)) {
    if (ListLength(node, mem) > 2) return CompileInfix(OpSeq(OpSub, mem), node, linkage, mem);
    else return CompilePrefix(OpNeg, node, linkage, mem);
  }
  if (IsTagged(node, "*", mem))     return CompileInfix(OpSeq(OpMul, mem), node, linkage, mem);
  if (IsTagged(node, "/", mem))     return CompileInfix(OpSeq(OpDiv, mem), node, linkage, mem);
  if (IsTagged(node, "==", mem))    return CompileInfix(OpSeq(OpEq, mem), node, linkage, mem);
  if (IsTagged(node, "!=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpEq, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, ">", mem))     return CompileInfix(OpSeq(OpGt, mem), node, linkage, mem);
  if (IsTagged(node, "<", mem))     return CompileInfix(OpSeq(OpLt, mem), node, linkage, mem);
  if (IsTagged(node, ">=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpLt, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, "<=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpGt, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, "not", mem))   return CompilePrefix(OpNot, node, linkage, mem);
  if (IsTagged(node, "in", mem))    return CompileInfix(OpSeq(OpIn, mem), node, linkage, mem);
  if (IsTagged(node, "|", mem))     return CompileInfix(OpSeq(OpPair, mem), node, linkage, mem);

  if (IsPair(node)) return CompileCall(node, linkage, mem);

  Print("Invalid AST: ");
  PrintVal(node, mem);
  Print("\n");
  Exit();
}
