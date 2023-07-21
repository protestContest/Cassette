#include "compile.h"
#include "parse.h"
#include "seq.h"
#include "assemble.h"
#include "ops.h"
#include "module.h"

static CompileResult CompileExpr(Val ast, Linkage linkage, Mem *mem);

static CompileResult CompileError(char *message, Val node)
{
  return (CompileResult){false, node, message, EmptySeq()};
}

static CompileResult CompileOk(Seq seq, Val node)
{
  return (CompileResult){true, node, "", seq};
}

CompileResult Compile(Val ast, Mem *mem)
{
  InitOps(mem);
  return CompileExpr(ast, LinkNext, mem);
}

static CompileResult CompileConst(Val node, Linkage linkage, Mem *mem)
{
  return CompileOk(EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node, nil, mem), mem)), mem), node);
}

static CompileResult CompileVar(Val node, Linkage linkage, Mem *mem)
{
  if (Eq(node, SymbolFor("true"))) {
    return CompileOk(EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpTrue), nil, mem)), mem), node);
  } else if (Eq(node, SymbolFor("false"))) {
    return CompileOk(EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpFalse), nil, mem)), mem), node);
  } else if (Eq(node, SymbolFor("nil"))) {
    return CompileOk(EndWithLinkage(linkage, MakeSeq(0, 0, Pair(OpSymbol(OpNil), nil, mem)), mem), node);
  }

  return CompileOk(EndWithLinkage(linkage,
    MakeSeq(RegEnv, 0,
      Pair(OpSymbol(OpLookup),
      Pair(node, nil, mem), mem)), mem), node);
}

static CompileResult CompileSymbol(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);
  return CompileOk(EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node, nil, mem), mem)), mem), node);
}

static CompileResult CompileString(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);
  return CompileOk(EndWithLinkage(linkage,
    MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(node,
      Pair(OpSymbol(OpStr), nil, mem), mem), mem)), mem), node);
}

static CompileResult CompileLambda(Val node, Linkage linkage, Mem *mem)
{
  Val params = ListAt(node, 1, mem);
  CompileResult body = CompileExpr(ListAt(node, 2, mem), LinkReturn, mem);
  if (!body.ok) return body;

  Seq param_code = EmptySeq();
  u32 arity = 0;
  while (!IsNil(params)) {
    Seq param = MakeSeq(0, RegEnv,
      Pair(OpSymbol(OpDefine),
      Pair(Head(params, mem),
      Pair(OpSymbol(OpPop), nil, mem), mem), mem));
    param_code = Preserving(RegEnv | RegCont, param, param_code, mem);
    params = Tail(params, mem);
    arity++;
  }

  Val entry = MakeLabel(mem);
  Val after_lambda = MakeLabel(mem);

  Seq body_code =
    AppendSeq(LabelSeq(entry, mem),
    AppendSeq(param_code,
    AppendSeq(body.result,
              LabelSeq(after_lambda, mem), mem), mem), mem);

  if (Eq(linkage, LinkNext)) linkage = after_lambda;

  Seq lambda_code =
    EndWithLinkage(linkage,
      MakeSeq(RegEnv, 0,
        Pair(OpSymbol(OpLambda),
        Pair(LabelRef(entry, mem),
        Pair(IntVal(arity), nil, mem), mem), mem)), mem);

  return CompileOk(TackOnSeq(lambda_code, body_code, mem), node);
}

static Seq OpSeq(OpCode op, Mem *mem)
{
  return MakeSeq(0, 0, Pair(OpSymbol(op), nil, mem));
}

static CompileResult CompilePrefix(OpCode op, Val node, Linkage linkage, Mem *mem)
{
  CompileResult args = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  if (!args.ok) return args;

  return CompileOk(Preserving(RegCont, args.result, EndWithLinkage(linkage, OpSeq(op, mem), mem), mem), node);
}

static CompileResult CompileInfix(Seq op_seq, Val node, Linkage linkage, Mem *mem)
{
  CompileResult a_code = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  if (!a_code.ok) return a_code;
  CompileResult b_code = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);
  if (!b_code.ok) return b_code;
  Seq args = Preserving(RegEnv, a_code.result, b_code.result, mem);

  return CompileOk(Preserving(RegCont, args, EndWithLinkage(linkage, op_seq, mem), mem), node);
}

static CompileResult CompileCall(Seq args, u32 num_args, Val node, Linkage linkage, Mem *mem)
{
  if (Eq(linkage, LinkReturn)) {
    return CompileOk(Preserving(RegCont,
      args,
      MakeSeq(RegCont, RegEnv,
        Pair(OpSymbol(OpApply),
        Pair(IntVal(num_args), nil, mem), mem)), mem), node);
  } else {
    Val after_call = MakeLabel(mem);
    if (Eq(linkage, LinkNext)) linkage = after_call;

    return CompileOk(
      AppendSeq(args,
        MakeSeq(0, RegCont | RegEnv,
          Pair(OpSymbol(OpCont),
          Pair(LabelRef(linkage, mem),
          Pair(OpSymbol(OpApply),
          Pair(IntVal(num_args),
          Pair(Label(after_call, mem), nil, mem), mem), mem), mem), mem)), mem), node);
  }
}

static CompileResult CompileApplication(Val node, Linkage linkage, Mem *mem)
{
  Seq args = EmptySeq();

  u32 num_args = 0;
  while (!IsNil(node)) {
    CompileResult arg = CompileExpr(Head(node, mem), LinkNext, mem);
    if (!arg.ok) return arg;

    num_args++;
    args = Preserving(RegEnv, arg.result, args, mem);
    node = Tail(node, mem);
  }

  return CompileCall(args, num_args-1, node, linkage, mem);
}

static CompileResult CompileDo(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq stmts = EmptySeq();
  if (IsNil(node)) return CompileOk(EndWithLinkage(linkage, stmts, mem), node);

  while (!IsNil(Tail(node, mem))) {
    CompileResult stmt = CompileExpr(Head(node, mem), LinkNext, mem);
    if (!stmt.ok) return stmt;

    stmts = AppendSeq(
      Preserving(RegEnv, stmts, stmt.result, mem),
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)), mem);

    node = Tail(node, mem);
  }

  CompileResult last_stmt = CompileExpr(Head(node, mem), linkage, mem);
  if (!last_stmt.ok) return last_stmt;

  return CompileOk(Preserving(RegEnv | RegCont, stmts, last_stmt.result, mem), node);
}

static CompileResult CompileLet(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq defines = EmptySeq();

  while (!IsNil(node)) {
    Val assign = Head(node, mem);
    Val var = ListAt(assign, 0, mem);
    Val val = ListAt(assign, 1, mem);

    CompileResult value = CompileExpr(val, LinkNext, mem);
    if (!value.ok) return value;

    Seq def = Preserving(RegEnv,
      value.result,
      MakeSeq(RegEnv, 0,
        Pair(OpSymbol(OpDefine),
        Pair(var, nil, mem), mem)), mem);

    defines = Preserving(RegCont, defines, def, mem);
    node = Tail(node, mem);
  }

  return CompileOk(EndWithLinkage(linkage, defines, mem), node);
}

static CompileResult CompileIf(Val node, Linkage linkage, Mem *mem)
{
  Val t_branch = MakeLabel(mem);
  Val f_branch = MakeLabel(mem);
  Val after_if = MakeLabel(mem);
  Linkage cons_linkage = (Eq(linkage, LinkNext)) ? after_if : linkage;

  CompileResult predicate = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  if (!predicate.ok) return predicate;

  CompileResult consequent = CompileExpr(ListAt(node, 2, mem), cons_linkage, mem);
  if (!consequent.ok) return consequent;

  Seq c_code =
    AppendSeq(
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)),
      consequent.result, mem);

  CompileResult alternative = CompileExpr(ListAt(node, 3, mem), linkage, mem);
  if (!alternative.ok) return alternative;

  Seq a_code =
    AppendSeq(
      MakeSeq(0, 0, Pair(OpSymbol(OpPop), nil, mem)),
      alternative.result, mem);

  Seq test_seq = MakeSeq(0, 0,
    Pair(OpSymbol(OpBranchF),
    Pair(LabelRef(f_branch, mem), nil, mem), mem));

  return CompileOk(
    Preserving(RegEnv | RegCont, predicate.result,
      AppendSeq(
        AppendSeq(test_seq,
          ParallelSeq(
            AppendSeq(LabelSeq(t_branch, mem), c_code, mem),
            AppendSeq(LabelSeq(f_branch, mem), a_code, mem), mem), mem),
        LabelSeq(after_if, mem), mem), mem), node);
}

static CompileResult CompileLogic(Val node, Linkage linkage, Mem *mem)
{
  Val after = MakeLabel(mem);

  CompileResult a = CompileExpr(ListAt(node, 1, mem), LinkNext, mem);
  if (!a.ok) return a;

  CompileResult b = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);
  if (!b.ok) return b;

  OpCode op = IsTagged(node, "and", mem) ? OpBranchF : OpBranch;

  Seq test_seq = MakeSeq(0, 0,
    Pair(OpSymbol(op),
    Pair(LabelRef(after, mem),
    Pair(OpSymbol(OpPop), nil, mem), mem), mem));

  return CompileOk(
    EndWithLinkage(linkage,
      Preserving(RegEnv | RegCont, a.result,
        AppendSeq(
          AppendSeq(test_seq, b.result, mem),
          LabelSeq(after, mem), mem), mem), mem), node);
}

static CompileResult CompileList(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    CompileResult item = CompileExpr(Head(node, mem), LinkNext, mem);
    if (!item.ok) return item;

    num_items++;
    items = Preserving(RegEnv, items, item.result, mem);
    node = Tail(node, mem);
  }

  return CompileOk(
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpList),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem), node);
}

static CompileResult CompileTuple(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    CompileResult item = CompileExpr(Head(node, mem), LinkNext, mem);
    if (!item.ok) return item;

    num_items++;
    items = Preserving(RegEnv, items, item.result, mem);
    node = Tail(node, mem);
  }

  return CompileOk(
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpTuple),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem), node);
}

static CompileResult CompileMap(Val node, Linkage linkage, Mem *mem)
{
  node = Tail(node, mem);

  Seq items = EmptySeq();
  u32 num_items = 0;
  while (!IsNil(node)) {
    Val pair = Head(node, mem);
    Seq key = MakeSeq(0, 0,
      Pair(OpSymbol(OpConst),
      Pair(Head(pair, mem), nil, mem), mem));

    CompileResult value = CompileExpr(Tail(pair, mem), LinkNext, mem);
    if (!value.ok) return value;

    Seq item = AppendSeq(value.result, key, mem);
    num_items++;
    items = Preserving(RegEnv, items, item, mem);
    node = Tail(node, mem);
  }

  return CompileOk(
    Preserving(RegCont, items,
      EndWithLinkage(linkage,
        MakeSeq(0, 0,
          Pair(OpSymbol(OpMap),
          Pair(IntVal(num_items), nil, mem), mem)), mem), mem), node);
}

static CompileResult CompileImport(Val node, Linkage linkage, Mem *mem)
{
  Val name = ListAt(node, 1, mem);
  Val alias = ListAt(node, 2, mem);

  Seq load = MakeSeq(0, 0,
    Pair(OpSymbol(OpGetMod),
    Pair(name, nil, mem), mem));

  CompileResult call = CompileCall(EmptySeq(), 0, nil, linkage, mem);
  if (!call.ok) return call;

  Seq export =
    AppendSeq(
      call.result,
      MakeSeq(0, 0,
        Pair(OpSymbol(OpPop),
        Pair(OpSymbol(OpExport), nil, mem), mem)), mem);

  Seq import_seq = MakeSeq(RegEnv, 0,
      Pair(OpSymbol(OpImport),
      Pair(alias, nil, mem), mem));

  return CompileOk(
    AppendSeq(load,
    Preserving(RegEnv, export, import_seq, mem), mem), node);
}

static CompileResult CompileModule(Val node, Linkage linkage, Mem *mem)
{
  CompileResult mod_code = CompileExpr(ListAt(node, 2, mem), LinkNext, mem);
  if (!mod_code.ok) return mod_code;

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(mod_code.result,
        MakeSeq(0, 0,
        Pair(OpSymbol(OpDefMod),
        Pair(ListAt(node, 1, mem), nil, mem), mem)), mem), mem), node);
}

static CompileResult CompileEmpty(Val node, Linkage linkage, Mem *mem)
{
  return CompileOk(EndWithLinkage(linkage, EmptySeq(), mem), node);
}

static bool IsApplicable(Val node, Mem *mem)
{
  if (IsTagged(node, "->", mem)) return false;
  if (IsTagged(node, "#", mem))  return false;
  if (IsTagged(node, "not", mem)) return false;
  if (IsTagged(node, "^", mem)) return false;
  if (IsTagged(node, "*", mem)) return false;
  if (IsTagged(node, "/", mem)) return false;
  if (IsTagged(node, "+", mem)) return false;
  if (IsTagged(node, "-", mem)) return false;
  if (IsTagged(node, "|", mem)) return false;
  if (IsTagged(node, "in", mem)) return false;
  if (IsTagged(node, ">", mem)) return false;
  if (IsTagged(node, "<", mem)) return false;
  if (IsTagged(node, ">=", mem)) return false;
  if (IsTagged(node, "<=", mem)) return false;
  if (IsTagged(node, "==", mem)) return false;
  if (IsTagged(node, "!=", mem)) return false;

  return true;
}

static CompileResult CompileExpr(Val node, Linkage linkage, Mem *mem)
{
  if (IsNil(node))                    return CompileEmpty(node, linkage, mem);
  if (IsNumeric(node))                return CompileConst(node, linkage, mem);
  if (IsSym(node))                    return CompileVar(node, linkage, mem);
  if (IsTagged(node, "\"", mem))      return CompileString(node, linkage, mem);
  if (IsTagged(node, ":", mem))       return CompileSymbol(node, linkage, mem);
  if (IsTagged(node, "*list*", mem))  return CompileList(node, linkage, mem);
  if (IsTagged(node, "*tuple*", mem)) return CompileTuple(node, linkage, mem);
  if (IsTagged(node, "*map*", mem))   return CompileMap(node, linkage, mem);
  if (IsTagged(node, "do", mem))      return CompileDo(node, linkage, mem);
  if (IsTagged(node, "if", mem))      return CompileIf(node, linkage, mem);
  if (IsTagged(node, ".", mem))       return CompileInfix(OpSeq(OpAccess, mem), node, linkage, mem);
  if (IsTagged(node, "#", mem))       return CompilePrefix(OpLen, node, linkage, mem);
  if (IsTagged(node, "not", mem))     return CompilePrefix(OpNot, node, linkage, mem);
  if (IsTagged(node, "^", mem))       return CompileInfix(OpSeq(OpExp, mem), node, linkage, mem);
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
  if (IsTagged(node, "module", mem))  return CompileModule(node, linkage, mem);

  if (IsPair(node)) {
    if (IsApplicable(Head(node, mem), mem)) return CompileApplication(node, linkage, mem);
    else return CompileExpr(Head(node, mem), linkage, mem);
  }

  return CompileError("Invalid node", node);
}
