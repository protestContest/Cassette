#include "compile.h"
#include "parse.h"
#include "seq.h"
#include "assemble.h"
#include "ops.h"
#include "module.h"

static Seq CompileExpr(Val ast, Linkage linkage, Mem *mem);

Seq Compile(Val ast, Mem *mem)
{
  return CompileExpr(ast, LinkNext, mem);
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
  if (Eq(node, SymbolFor("true"))) {
    return EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpTrue), nil, mem)), mem);
  } else if (Eq(node, SymbolFor("false"))) {
    return EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpFalse), nil, mem)), mem);
  } else if (Eq(node, SymbolFor("nil"))) {
    return EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpNil), nil, mem)), mem);
  }

  return EndWithLinkage(linkage,
    MakeSeq(RegEnv, 0,
      Pair(OpSymbol(OpLookup),
      Pair(node, nil, mem), mem)), mem);
}

static Seq CompileSymbol(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);
  return EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node, nil, mem), mem)), mem);
}

static Seq CompileString(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);
  return EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node,
      Pair(OpSymbol(OpStr), nil, mem), mem), mem)), mem);
}

static Seq CompileLambda(Val node, Linkage linkage, Mem *mem)
{
  Val params = ListAt(node, 1, mem);
  Seq body = CompileExpr(ListAt(node, 2, mem), LinkReturn, mem);

  Seq param_code = EmptySeq();
  while (!IsNil(params)) {
    Seq param = MakeSeq(0, RegEnv,
      Pair(OpSymbol(OpDefine),
      Pair(Head(params, mem),
      Pair(OpSymbol(OpPop), nil, mem), mem), mem));
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

  while (!IsNil(node)) {
    Seq arg = CompileExpr(Head(node, mem), LinkNext, mem);
    args = Preserving(RegEnv, arg, args, mem);
    node = Tail(node, mem);
  }

  if (Eq(linkage, LinkReturn)) {
    return Preserving(RegCont,
      args,
      MakeSeq(RegCont, RegEnv,
        Pair(OpSymbol(OpApply), nil, mem)), mem);
  } else {
    Val after_call = MakeLabel(mem);
    if (Eq(linkage, LinkNext)) linkage = after_call;

    return
      AppendSeq(args,
        MakeSeq(0, RegCont | RegEnv,
          Pair(OpSymbol(OpCont),
          Pair(LabelRef(linkage, mem),
          Pair(OpSymbol(OpApply),
          Pair(Label(after_call, mem), nil, mem), mem), mem), mem)), mem);
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

    Linkage def_linkage = (IsNil(Tail(node, mem))) ? linkage : LinkNext;

    Seq def = Preserving(RegEnv,
      CompileExpr(Tail(pair, mem), def_linkage, mem),
      MakeSeq(RegEnv, 0,
        Pair(OpSymbol(OpDefine),
        Pair(var, nil, mem), mem)), mem);

    defines = Preserving(RegCont, defines, def, mem);
    node = Tail(node, mem);
  }

  return defines;
}

static Seq CompileIf(Val node, Linkage linkage, Mem *mem)
{
  Assert(IsTagged(node, "if", mem));

  Val t_branch = MakeLabel(mem);
  Val f_branch = MakeLabel(mem);
  Val after_if = MakeLabel(mem);
  Linkage cons_linkage = (Eq(linkage, LinkNext)) ? after_if : linkage;
  Seq predicate = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  Seq consequent =
    AppendSeq(
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)),
      CompileExpr(ListAt(node, 2, mem), cons_linkage, mem), mem);
  Seq alternative =
    AppendSeq(
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)),
      CompileExpr(ListAt(node, 3, mem), linkage, mem), mem);

  Seq test_seq = MakeSeq(0, 0,
    Pair(OpSymbol(OpBranchF),
    Pair(LabelRef(f_branch, mem), nil, mem), mem));

  return
    Preserving(RegEnv | RegCont, predicate,
      AppendSeq(
        AppendSeq(test_seq,
          ParallelSeq(
            AppendSeq(LabelSeq(t_branch, mem), consequent, mem),
            AppendSeq(LabelSeq(f_branch, mem), alternative, mem), mem), mem),
        LabelSeq(after_if, mem), mem), mem);
}

static Seq CompileCond(Val node, Linkage linkage, Mem *mem)
{
  return EmptySeq();
}

static Seq CompileLogic(Val node, Linkage linkage, Mem *mem)
{
  Val after = MakeLabel(mem);

  Seq a = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  Seq b = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);

  OpCode op = IsTagged(node, "and", mem) ? OpBranchF : OpBranch;

  Seq test_seq = MakeSeq(0, 0,
    Pair(OpSymbol(op),
    Pair(LabelRef(after, mem),
    Pair(OpSymbol(OpPop), nil, mem), mem), mem));

  return EndWithLinkage(linkage,
    Preserving(RegEnv | RegCont, a,
      AppendSeq(
        AppendSeq(test_seq, b, mem),
        LabelSeq(after, mem), mem), mem), mem);
}

static Seq CompileList(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    Seq item = CompileExpr(Head(node, mem), LinkNext, mem);
    num_items++;
    items = Preserving(RegEnv, items, item, mem);
    node = Tail(node, mem);
  }

  return
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpList),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem);
}

static Seq CompileTuple(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    Seq item = CompileExpr(Head(node, mem), LinkNext, mem);
    num_items++;
    items = Preserving(RegEnv, items, item, mem);
    node = Tail(node, mem);
  }

  return
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpTuple),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem);
}

static Seq CompileMap(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    Val pair = Head(node, mem);
    Seq key = MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(Head(pair, mem), nil, mem), mem));

    Seq item = AppendSeq(
      CompileExpr(Tail(pair, mem), LinkNext, mem),
      key, mem);
    num_items++;
    items = Preserving(RegEnv, items, item, mem);
    node = Tail(node, mem);
  }

  return
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpMap),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem);
}

static Seq CompileImport(Val node, Linkage linkage, Mem *mem)
{
  return EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpImport),
      Pair(Tail(node, mem), nil, mem), mem)), mem);
}

static Seq CompileModule(Val node, Linkage linkage, Mem *mem)
{
  Seq mod_code = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);
  PrintVal(node, mem);
  Print("\n");
  return EndWithLinkage(linkage,
    AppendSeq(mod_code,
      MakeSeq(0, 0,
      Pair(OpSymbol(OpDefMod),
      Pair(ListAt(node, 1, mem), nil, mem), mem)), mem), mem);
}

static Seq CompileExpr(Val node, Linkage linkage, Mem *mem)
{
  // Print("=> ");
  // PrintVal(node, mem);
  // Print("\n");

  if (IsNil(node))                    return EmptySeq();
  if (IsNumeric(node))                return CompileConst(node, linkage, mem);
  if (IsSym(node))                    return CompileVar(node, linkage, mem);
  if (IsTagged(node, "\"", mem))      return CompileString(node, linkage, mem);
  if (IsTagged(node, ":", mem))       return CompileSymbol(node, linkage, mem);
  if (IsTagged(node, "[", mem))       return CompileList(node, linkage, mem);
  if (IsTagged(node, "#[", mem))      return CompileTuple(node, linkage, mem);
  if (IsTagged(node, "{", mem))       return CompileMap(node, linkage, mem);
  if (IsTagged(node, "do", mem))      return CompileDo(node, linkage, mem);
  if (IsTagged(node, "if", mem))      return CompileIf(node, linkage, mem);
  if (IsTagged(node, ".", mem))       return CompileInfix(OpSeq(OpAccess, mem), node, linkage, mem);
  if (IsTagged(node, "not", mem))     return CompilePrefix(OpNot, node, linkage, mem);
  if (IsTagged(node, "*", mem))       return CompileInfix(OpSeq(OpMul, mem), node, linkage, mem);
  if (IsTagged(node, "/", mem))       return CompileInfix(OpSeq(OpDiv, mem), node, linkage, mem);
  if (IsTagged(node, "+", mem))       return CompileInfix(OpSeq(OpAdd, mem), node, linkage, mem);
  if (IsTagged(node, "-", mem)) {
    if (ListLength(node, mem) > 2)    return CompileInfix(OpSeq(OpSub, mem), node, linkage, mem);
    else                              return CompilePrefix(OpNeg, node, linkage, mem);
  }
  if (IsTagged(node, "|", mem))       return CompileInfix(OpSeq(OpPair, mem), node, linkage, mem);
  if (IsTagged(node, "in", mem))      return CompileInfix(OpSeq(OpIn, mem), node, linkage, mem);
  if (IsTagged(node, ">", mem))       return CompileInfix(OpSeq(OpGt, mem), node, linkage, mem);
  if (IsTagged(node, "<", mem))       return CompileInfix(OpSeq(OpLt, mem), node, linkage, mem);
  if (IsTagged(node, ">=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpLt, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, "<=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpGt, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, "==", mem))      return CompileInfix(OpSeq(OpEq, mem), node, linkage, mem);
  if (IsTagged(node, "!=", mem)) {
    Seq op_seq = AppendSeq(OpSeq(OpEq, mem), OpSeq(OpNot, mem), mem);
    return CompileInfix(op_seq, node, linkage, mem);
  }
  if (IsTagged(node, "or", mem))      return CompileLogic(node, linkage, mem);
  if (IsTagged(node, "and", mem))     return CompileLogic(node, linkage, mem);

  if (IsTagged(node, "->", mem))      return CompileLambda(node, linkage, mem);
  if (IsTagged(node, "let", mem))     return CompileLet(node, linkage, mem);
  if (IsTagged(node, "import", mem))  return CompileImport(node, linkage, mem);
  if (IsTagged(node, "defmod", mem))  return CompileModule(node, linkage, mem);

  if (IsPair(node)) return CompileCall(node, linkage, mem);

  Print("Invalid AST: ");
  PrintVal(node, mem);
  Print("\n");
  Exit();
}
