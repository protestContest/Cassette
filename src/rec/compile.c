#include "compile.h"
#include "debug.h"
#include "parse.h"
#include "vm.h"
#include "source.h"
#include "module.h"
#include "univ/hashmap.h"
#include "univ/system.h"

#define CompileOk(seq)            ((CompileResult){true, {seq}})
#define ErrorResult(m, e, p)      ((CompileResult){false, {.error = {m, e, p}}})

typedef struct {
  Heap *mem;
  Val env;
  Module module;
  u32 pos;
  CassetteOpts *opts;
} Compiler;

static void InitCompiler(Compiler *c, CassetteOpts *opts, Val env, Heap *mem)
{
  c->mem = mem;
  c->env = env,
  c->module.name = nil;
  c->module.code = EmptySeq();
  c->module.imports = EmptyHashMap;
  c->pos = 0;
  c->opts = opts;

  Assert(Eq(LinkReturn, MakeSymbol("*return*", mem)));
  Assert(Eq(LinkNext, MakeSymbol("*next*", mem)));
  Assert(Eq(Ok, MakeSymbol("ok", mem)));
}

#define OpSeq(op, mem)  MakeSeq(0, 0, Pair(IntVal(op), nil, mem))

static CompileResult CompileExpr(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileConst(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileString(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileVar(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileList(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileTuple(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileMap(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileOr(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileAnd(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileIf(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileDo(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileLambda(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileModule(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileOp(Seq op_seq, Val expr, Linkage linkage, Compiler *c);
static CompileResult CompilePair(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileApplication(Val expr, Linkage linkage, Compiler *c);

ModuleResult Compile(Val ast, CassetteOpts *opts, Val env, Heap *mem)
{
  Compiler c;
  InitCompiler(&c, opts, env, mem);
  CompileResult result = CompileExpr(ast, LinkNext, &c);
  if (!result.ok) {
    return (ModuleResult){false, {.error = result.error}};
  }
  c.module.code = result.code;
  return (ModuleResult){true, {.module = c.module}};
}

static CompileResult CompileExpr(Val ast, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  // if (c->args->verbose > 1) {
  //   PrintInt(c->pos);
  //   Print("#> ");
    // PrintAST(expr, 3, mem);
  //   Print("\n");
  // }

  c->pos = RawInt(Head(ast, mem));
  Val expr = Tail(ast, mem);

  CompileResult result;

  if (IsNil(expr))                          result = CompileOk(EndWithLinkage(linkage, EmptySeq(), mem));
  else if (IsNum(expr))                     result = CompileConst(expr, linkage, c);
  else if (IsTagged(expr, ":", mem))        result = CompileConst(Tail(Tail(expr, mem), mem), linkage, c);
  else if (IsTagged(expr, "\"", mem))       result = CompileString(Tail(expr, mem), linkage, c);
  else if (Eq(expr, NilTag))                result = CompileConst(nil, linkage, c);
  else if (Eq(expr, True))                  result = CompileConst(expr, linkage, c);
  else if (Eq(expr, False))                 result = CompileConst(expr, linkage, c);
  else if (IsSym(expr))                     result = CompileVar(expr, linkage, c);

  else if (IsTagged(expr, "[", mem))        result = CompileList(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "{", mem))        result = CompileTuple(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "{:", mem))       result = CompileMap(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "or", mem))       result = CompileOr(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "and", mem))      result = CompileAnd(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "if", mem))       result = CompileIf(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "do", mem))       result = CompileDo(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "->", mem))       result = CompileLambda(Tail(expr, mem), linkage, c);
  else if (IsTagged(expr, "module", mem))   result = CompileModule(Tail(expr, mem), linkage, c);

  else if (IsTagged(expr, ".", mem))        result = CompileOp(OpSeq(OpGet, mem), expr, linkage, c);
  else if (IsTagged(expr, "not", mem))      result = CompileOp(OpSeq(OpNot, mem), expr, linkage, c);
  else if (IsTagged(expr, "#", mem))        result = CompileOp(OpSeq(OpLen, mem), expr, linkage, c);
  else if (IsTagged(expr, "*", mem))        result = CompileOp(OpSeq(OpMul, mem), expr, linkage, c);
  else if (IsTagged(expr, "/", mem))        result = CompileOp(OpSeq(OpDiv, mem), expr, linkage, c);
  else if (IsTagged(expr, "%", mem))        result = CompileOp(OpSeq(OpRem, mem), expr, linkage, c);
  else if (IsTagged(expr, "+", mem))        result = CompileOp(OpSeq(OpAdd, mem), expr, linkage, c);
  else if (IsTagged(expr, "-", mem)) {
    if (ListLength(expr, mem) == 2)         result = CompileOp(OpSeq(OpNeg, mem), expr, linkage, c);
    else                                    result = CompileOp(OpSeq(OpSub, mem), expr, linkage, c);
  }
  else if (IsTagged(expr, "|", mem))        result = CompilePair(expr, linkage, c);
  else if (IsTagged(expr, "in", mem))       result = CompileOp(OpSeq(OpIn, mem), expr, linkage, c);
  else if (IsTagged(expr, ">", mem))        result = CompileOp(OpSeq(OpGt, mem), expr, linkage, c);
  else if (IsTagged(expr, "<", mem))        result = CompileOp(OpSeq(OpLt, mem), expr, linkage, c);
  else if (IsTagged(expr, "==", mem))       result = CompileOp(OpSeq(OpEq, mem), expr, linkage, c);

  else if (IsPair(expr))                    result = CompileApplication(expr, linkage, c);
  else                                      result = ErrorResult("Unknown expression", NULL, c->pos);

  return result;
}

static CompileResult CompileConst(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  return CompileOk(
    EndWithLinkage(linkage,
      MakeSeq(0, 0,
        Pair(SourceRef(c->pos, mem),
        Pair(IntVal(OpConst),
        Pair(expr, nil, mem), mem), mem)), mem));
}

static CompileResult CompileString(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  CompileResult item = CompileConst(expr, linkage, c);
  if (!item.ok) return item;

  return CompileOk(
    AppendSeq(item.code,
    MakeSeq(0, 0,
      Pair(IntVal(OpStr), nil, mem)), mem));
}

static CompileResult CompileVar(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  Val pos = FindVar(expr, c->env, mem);
  if (IsNil(pos)) {
    return ErrorResult("Undefined variable", NULL, c->pos);
  }

  return CompileOk(
    EndWithLinkage(linkage,
      MakeSeq(REnv, 0,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpLookup),
        Pair(Head(pos, mem),
        Pair(Tail(pos, mem), nil, mem), mem), mem), mem)), mem));
}

static CompileResult CompileList(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  Seq seq = EmptySeq();
  while (!IsNil(expr)) {
    CompileResult item = CompileExpr(Head(expr, mem), LinkNext, c);
    if (!item.ok) return item;

    seq =
      Preserving(REnv, item.code,
      AppendSeq(
        MakeSeq(0, 0, Pair(IntVal(OpPair), nil, mem)),
        seq, mem), mem);

    expr = Tail(expr, mem);
  }

  seq = AppendSeq(
    MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(IntVal(OpConst),
      Pair(nil, nil, mem), mem), mem)),
    seq, mem);

  return CompileOk(EndWithLinkage(linkage, seq, mem));
}

static CompileResult CompileTuple(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  u32 num_items = 0;
  Seq items_seq = EmptySeq();
  while (!IsNil(expr)) {
    CompileResult item = CompileExpr(Head(expr, mem), LinkNext, c);
    if (!item.ok) return item;

    items_seq = Preserving(REnv,
      AppendSeq(item.code,
        MakeSeq(0, 0,
          Pair(IntVal(OpSet),
          Pair(IntVal(num_items), nil, mem), mem)), mem),
      items_seq, mem);
    num_items++;

    expr = Tail(expr, mem);
  }

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(
        MakeSeq(0, 0,
          Pair(SourceRef(source, mem),
          Pair(IntVal(OpTuple),
          Pair(IntVal(num_items), nil, mem), mem), mem)),
        items_seq, mem), mem));
}

static CompileResult CompileMap(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  u32 num_items = 0;
  Seq keys_seq = EmptySeq();
  Seq vals_seq = EmptySeq();
  while (!IsNil(expr)) {
    Val item = Head(expr, mem);
    Val key = Tail(Head(item, mem), mem);

    keys_seq =
      AppendSeq(
        MakeSeq(0, 0,
        Pair(IntVal(OpConst),
        Pair(key,
        Pair(IntVal(OpSet),
        Pair(IntVal(num_items), nil, mem), mem), mem), mem)),
      keys_seq, mem);

    CompileResult val = CompileExpr(Tail(item, mem), LinkNext, c);
    if (!val.ok) return val;

    vals_seq =
      Preserving(REnv,
        AppendSeq(val.code,
        MakeSeq(0, 0,
          Pair(IntVal(OpSet),
          Pair(IntVal(num_items), nil, mem), mem)), mem),
      vals_seq, mem);

    num_items++;
    expr = Tail(expr, mem);
  }

  vals_seq =
    AppendSeq(
      MakeSeq(0, 0,
        Pair(IntVal(OpTuple),
        Pair(IntVal(num_items), nil, mem), mem)),
      vals_seq, mem);

  keys_seq =
    AppendSeq(
      MakeSeq(0, 0,
        Pair(IntVal(OpTuple),
        Pair(IntVal(num_items), nil, mem), mem)),
      keys_seq, mem);

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(
        AppendSeq(vals_seq, keys_seq, mem),
        MakeSeq(0, 0,
          Pair(SourceRef(source, mem),
          Pair(IntVal(OpMap), nil, mem), mem)), mem), mem));
}

static CompileResult CompileItems(Val items, Compiler *c)
{
  Heap *mem = c->mem;

  Seq items_seq = EmptySeq();
  while (!IsNil(items)) {
    CompileResult item = CompileExpr(Head(items, mem), LinkNext, c);
    if (!item.ok) return item;

    items_seq = Preserving(REnv, item.code, items_seq, mem);
    items = Tail(items, mem);
  }

  return CompileOk(items_seq);
}

static CompileResult CompileOp(Seq op_seq, Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  CompileResult args = CompileItems(ReverseList(Tail(expr, mem), mem), c);
  if (!args.ok) return args;

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(args.code,
      AppendSeq(SourceSeq(source, mem),
      op_seq, mem), mem), mem));
}

static CompileResult CompilePair(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  CompileResult args = CompileItems(Tail(expr, mem), c);
  if (!args.ok) return args;

  Seq op_seq = OpSeq(OpPair, mem);
  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(args.code,
      AppendSeq(SourceSeq(source, mem),
      op_seq, mem), mem), mem));
}

static CompileResult CompileOr(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  CompileResult a = CompileExpr(ListAt(expr, 0, mem), LinkNext, c);
  if (!a.ok) return a;

  Val after_b = MakeLabel();

  CompileResult b = CompileExpr(ListAt(expr, 1, mem), linkage, c);
  if (!b.ok) return b;

  Val true_branch = linkage;
  if (Eq(linkage, LinkNext) || Eq(linkage, LinkReturn)) {
    true_branch = after_b;
  }

  return CompileOk(
    Preserving(REnv | RCont,
      a.code,
      EndWithLinkage(linkage,
        AppendSeq(
          MakeSeq(0, 0,
            Pair(SourceRef(source, mem),
            Pair(IntVal(OpBranch),
            Pair(LabelRef(true_branch, mem),
            Pair(IntVal(OpPop), nil, mem), mem), mem), mem)),
        AppendSeq(b.code,
          LabelSeq(after_b, mem), mem), mem), mem), mem));
}

static CompileResult CompileAnd(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  CompileResult a = CompileExpr(ListAt(expr, 0, mem), LinkNext, c);
  if (!a.ok) return a;

  Val true_branch = MakeLabel();
  Val after_b = MakeLabel();

  CompileResult b = CompileExpr(ListAt(expr, 1, mem), linkage, c);
  if (!b.ok) return b;

  Val a_linkage = Eq(linkage, LinkNext) ? after_b : linkage;

  return CompileOk(
    Preserving(REnv | RCont,
      a.code,
      AppendSeq(
        EndWithLinkage(a_linkage,
          MakeSeq(0, 0,
            Pair(SourceRef(source, mem),
            Pair(IntVal(OpBranch),
            Pair(LabelRef(true_branch, mem), nil, mem), mem), mem)), mem),
      AppendSeq(
        MakeSeq(0, 0,
          Pair(SourceRef(source, mem),
          Pair(Label(true_branch, mem),
          Pair(IntVal(OpPop), nil, mem), mem), mem)),
      AppendSeq(b.code,
        LabelSeq(after_b, mem), mem), mem), mem), mem));
}

static CompileResult CompileIf(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  CompileResult predicate = CompileExpr(ListAt(expr, 0, mem), LinkNext, c);
  if (!predicate.ok) return predicate;

  Val true_branch = MakeLabel();
  Val after = MakeLabel();

  Seq pred_branch =
      MakeSeq(0, 0,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpBranch),
        Pair(LabelRef(true_branch, mem), nil, mem), mem), mem));

  Val alt_linkage = Eq(linkage, LinkNext) ? after : linkage;
  CompileResult alternative = CompileExpr(ListAt(expr, 2, mem), alt_linkage, c);
  if (!alternative.ok) return alternative;

  Seq alt_seq =
    AppendSeq(MakeSeq(0, 0, Pair(IntVal(OpPop), nil, mem)),
    alternative.code, mem);

  CompileResult consequent = CompileExpr(ListAt(expr, 1, mem), linkage, c);
  if (!consequent.ok) return consequent;

  Seq cons_seq =
    AppendSeq(MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(Label(true_branch, mem),
      Pair(IntVal(OpPop), nil, mem), mem), mem)),
    AppendSeq(consequent.code,
    LabelSeq(after, mem), mem), mem);

  return CompileOk(
    AppendSeq(SourceSeq(source, mem),
    Preserving(REnv | RCont,
      predicate.code,
      AppendSeq(pred_branch,
        ParallelSeq(alt_seq, cons_seq, mem), mem), mem), mem));
}

static CompileResult CompileAssigns(Val expr, u32 index, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  Seq assigns = EmptySeq();
  while (!IsNil(expr)) {
    Val assign = Head(expr, mem);
    Val var = Tail(ListAt(assign, 0, mem), mem);
    Define(index, var, c->env, mem);

    CompileResult value = CompileExpr(ListAt(assign, 1, mem), LinkNext, c);
    if (!value.ok) return value;

    assigns =
      AppendSeq(assigns,
        Preserving(REnv, value.code,
          MakeSeq(REnv, 0,
            Pair(SourceRef(source, mem),
            Pair(IntVal(OpDefine),
            Pair(IntVal(index), nil, mem), mem), mem)), mem), mem);

    index++;
    expr = Tail(expr, mem);
  }

  return CompileOk(
    EndWithLinkage(linkage,
    AppendSeq(assigns,
    MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(IntVal(OpConst),
      Pair(Ok, nil, mem), mem), mem)), mem), mem));
}

static CompileResult CompileImport(Val expr, u32 index, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  Seq assigns = EmptySeq();
  while (!IsNil(expr)) {
    Val import = Head(expr, mem);
    Val mod = Tail(ListAt(import, 0, mem), mem);
    Val var = Tail(ListAt(import, 1, mem), mem);
    Define(index, var, c->env, mem);
    if (!HashMapContains(&c->module.imports, mod.as_i)) {
      HashMapSet(&c->module.imports, mod.as_i, 1);
    }

    Val after_load = MakeLabel();
    Seq load_seq =
      MakeSeq(0, RCont | REnv,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpLoad),
        Pair(ModuleRef(mod, mem),
        Pair(IntVal(OpCont),
        Pair(LabelRef(after_load, mem),
        Pair(IntVal(OpApply),
        Pair(Label(after_load, mem), nil, mem), mem), mem), mem), mem), mem), mem));

    assigns =
      AppendSeq(assigns,
      Preserving(REnv, load_seq,
      MakeSeq(REnv, 0,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpDefine),
        Pair(IntVal(index), nil, mem), mem), mem)), mem), mem);

    index++;
    expr = Tail(expr, mem);
  }

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(SourceSeq(source, mem),
      AppendSeq(assigns,
        MakeSeq(0, 0,
          Pair(IntVal(OpConst),
          Pair(Ok, nil, mem), mem)), mem), mem), mem));
}

static CompileResult CompileBlock(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;

  if (IsNil(expr)) {
    return CompileOk(MakeSeq(0, 0, Pair(IntVal(OpConst), Pair(nil, nil, mem), mem)));
  }

  Seq seq = EmptySeq();
  u32 index = 0;
  while (!IsNil(expr)) {
    Val stmt = Head(expr, mem);

    Val stmt_linkage = IsNil(Tail(expr, mem)) ? linkage : LinkNext;

    CompileResult stmt_res;
    if (IsTagged(Tail(stmt, mem), "let", mem)) {
      stmt_res = CompileAssigns(Tail(Tail(stmt, mem), mem), index, stmt_linkage, c);
      index += ListLength(Tail(Tail(stmt, mem), mem), mem);
    } else if (IsTagged(Tail(stmt, mem), "import", mem)) {
      stmt_res = CompileImport(Tail(Tail(stmt, mem), mem), index, stmt_linkage, c);
      index += ListLength(Tail(Tail(stmt, mem), mem), mem);
    } else {
      stmt_res = CompileExpr(stmt, stmt_linkage, c);
    }
    if (!stmt_res.ok) return stmt_res;

    Seq stmt_seq = stmt_res.code;
    u32 preserve = REnv;

    if (IsNil(Tail(expr, mem))) {
      preserve |= RCont;
    } else {
      stmt_seq = AppendSeq(stmt_seq,
        MakeSeq(0, 0,
          Pair(IntVal(OpPop), nil, mem)), mem);
    }

    seq = Preserving(preserve, seq, stmt_seq, mem);
    expr = Tail(expr, mem);
  }

  if (index > 0) {
    seq = AppendSeq(
      MakeSeq(0, REnv,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpTuple),
        Pair(IntVal(index),
        Pair(IntVal(OpExtend), nil, mem), mem), mem), mem)),
      seq, mem);
  }

  return CompileOk(seq);
}

static u32 CountAssigns(Val stmts, Heap *mem)
{
  u32 num_assigns = 0;
  while (!IsNil(stmts)) {
    Val stmt = Tail(Head(stmts, mem), mem);
    if (IsTagged(stmt, "let", mem) || IsTagged(stmt, "import", mem)) {
      num_assigns += ListLength(Tail(stmt, mem), mem);
    }
    stmts = Tail(stmts, mem);
  }
  return num_assigns;
}

static CompileResult CompileDo(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  u32 num_assigns = CountAssigns(expr, mem);

  if (num_assigns > 0) {
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, mem), mem);
  }

  CompileResult block = CompileBlock(expr, linkage, c);

  if (num_assigns > 0) {
    c->env = RestoreEnv(c->env, mem);
  }

  return block;
}

static CompileResult CompileLambda(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;
  Val params = Tail(ListAt(expr, 0, mem), mem);
  Val body = ListAt(expr, 1, mem);

  Seq params_code;
  if (IsSym(params)) {
    c->env = ExtendEnv(c->env, MakeTuple(1, mem), mem);
    Define(0, params, c->env, mem);

    params_code = MakeSeq(0, 0,
      Pair(IntVal(OpTuple),
      Pair(IntVal(1),
      Pair(IntVal(OpExtend),
      Pair(IntVal(OpDefine),
      Pair(IntVal(0), nil, mem), mem), mem), mem), mem));
  } else {
    u32 num_params = ListLength(params, mem);
    c->env = ExtendEnv(c->env, MakeTuple(num_params, mem), mem);
    for (u32 i = 0; i < num_params; i++) {
      Val param = Tail(Head(params, mem), mem);
      Define(i, param, c->env, mem);
      params = Tail(params, mem);
    }

    params_code = MakeSeq(0, 0,
      Pair(IntVal(OpExtend), nil, mem));
  }

  CompileResult body_code = CompileExpr(body, LinkReturn, c);
  if (!body_code.ok) return body_code;

  c->env = RestoreEnv(c->env, mem);

  Val after_lambda = MakeLabel();

  return CompileOk(
    EndWithLinkage(linkage,
    TackOnSeq(
      MakeSeq(0, 0,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpLambda),
        Pair(LabelRef(after_lambda, mem), nil, mem), mem), mem)),
    AppendSeq(params_code,
    AppendSeq(body_code.code,
    LabelSeq(after_lambda, mem), mem), mem), mem), mem));
}

static CompileResult CompileModule(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = c->pos;
  Val name = Tail(ListAt(expr, 0, mem), mem);
  c->module.name = name;

  Val after = MakeLabel();

  Val stmts = ListAt(expr, 1, mem);
  u32 num_assigns = CountAssigns(stmts, mem);

  if (num_assigns > 0) {
    c->env = ExtendEnv(c->env, MakeTuple(num_assigns, mem), mem);
  }

  CompileResult block = CompileBlock(stmts, LinkNext, c);
  if (!block.ok) return block;

  Seq keys_seq = MakeSeq(0, 0,
    Pair(IntVal(OpPop),
    Pair(IntVal(OpExport),
    Pair(IntVal(OpTuple),
    Pair(IntVal(num_assigns), nil, mem), mem), mem), mem));
  Val frame = Head(c->env, mem);
  for (u32 i = 0; i < num_assigns; i++) {
    Seq key_seq = MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(IntVal(OpConst),
      Pair(TupleGet(frame, i, mem),
      Pair(IntVal(OpSet),
      Pair(IntVal(i), nil, mem), mem), mem), mem), mem));
    keys_seq = AppendSeq(keys_seq, key_seq, mem);
  }

  Seq mod_body = EndWithLinkage(LinkReturn,
    AppendSeq(block.code,
    AppendSeq(keys_seq,
    MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(IntVal(OpMap),
      Pair(IntVal(OpDup),
      Pair(IntVal(OpModule),
      Pair(ModuleRef(name, mem), nil, mem), mem), mem), mem), mem)), mem), mem), mem);

  return CompileOk(
    EndWithLinkage(linkage,
    AppendSeq(
      TackOnSeq(MakeSeq(0, 0,
        Pair(SourceRef(source, mem),
        Pair(IntVal(OpLambda),
        Pair(LabelRef(after, mem), nil, mem), mem), mem)),
      mod_body, mem),
    MakeSeq(0, 0,
      Pair(SourceRef(source, mem),
      Pair(Label(after, mem),
      Pair(IntVal(OpModule),
      Pair(ModuleRef(name, mem), nil, mem), mem), mem), mem)), mem), mem));
}

static CompileResult CompileApplication(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  u32 source = RawInt(Head(Head(expr, mem), mem));

  CompileResult op = CompileExpr(Head(expr, mem), LinkNext, c);
  if (!op.ok) return op;

  CompileResult args = CompileTuple(Tail(expr, mem), LinkNext, c);
  if (!args.ok) return args;

  Seq call = Preserving(REnv, args.code, op.code, mem);

  if (Eq(linkage, LinkReturn)) {
    return CompileOk(
      Preserving(RCont,
        call,
        MakeSeq(RCont, REnv,
          Pair(SourceRef(source, mem),
          Pair(IntVal(OpApply), nil, mem), mem)), mem));
  }

  Val after_call = MakeLabel();
  if (Eq(linkage, LinkNext)) linkage = after_call;

  return CompileOk(
    AppendSeq(call,
    MakeSeq(0, REnv | RCont,
      Pair(SourceRef(source, mem),
      Pair(IntVal(OpCont),
      Pair(LabelRef(linkage, mem),
      Pair(IntVal(OpApply),
      Pair(Label(after_call, mem), nil, mem), mem), mem), mem), mem)), mem));
}
