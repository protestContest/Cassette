#include "compile.h"
#include "debug.h"
#include "vm.h"

typedef struct {
  bool ok;
  union {
    Seq result;
    char *error;
  };
} CompileResult;

#define CompileOk(seq)      ((CompileResult){true, {seq}})
#define CompileError(err)   ((CompileResult){false, {.error = err}})

typedef struct {
  Heap *mem;
  Val env;
  Module module;
} Compiler;

static void InitCompiler(Compiler *c, Val env, Heap *mem)
{
  c->mem = mem;
  if (IsNil(env)) {
    c->env = CompileEnv(mem);
  } else {
    c->env = env;
  }
  c->module.name = nil;
  c->module.code = EmptySeq();
  c->module.imports = EmptyHashMap;

  MakeSymbol("return", mem);
  MakeSymbol("next", mem);
}

#define OpSeq(op, mem)  MakeSeq(0, 0, Pair(IntVal(op), nil, mem))

static CompileResult CompileExpr(Val expr, Linkage linkage, Compiler *c);
static CompileResult CompileConst(Val expr, Linkage linkage, Compiler *c);
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
static CompileResult CompileApplication(Val expr, Linkage linkage, Compiler *c);

ModuleResult Compile(Val ast, Val env, Heap *mem)
{
  Compiler c;
  InitCompiler(&c, env, mem);
  CompileResult compiled = CompileExpr(ast, LinkNext, &c);

  if (compiled.ok) {
    c.module.code = compiled.result;
    return (ModuleResult){true, {c.module}};
  } else {
    return (ModuleResult){false, {.error = compiled.error}};
  }
}

static CompileResult CompileExpr(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  // Val pos = Head(expr, mem);
  expr = Tail(expr, mem);

  // PrintInt(RawInt(pos));
  // Print("#> ");
  // Inspect(expr, mem);
  // Print("\n  ");
  // Inspect(c->env, mem);
  // Print("\n");

  if (IsNil(expr))                      return CompileOk(EndWithLinkage(linkage, EmptySeq(), mem));
  if (IsNum(expr))                      return CompileConst(expr, linkage, c);
  if (IsTagged(expr, ":", mem))         return CompileConst(Tail(Tail(expr, mem), mem), linkage, c);
  if (IsTagged(expr, "\"", mem))        return CompileConst(ListAt(expr, 1, mem), linkage, c);
  if (Eq(expr, SymbolFor("nil")))       return CompileConst(nil, linkage, c);
  if (Eq(expr, SymbolFor("true")))      return CompileConst(expr, linkage, c);
  if (Eq(expr, SymbolFor("false")))     return CompileConst(expr, linkage, c);
  if (IsSym(expr))                      return CompileVar(expr, linkage, c);

  if (IsTagged(expr, "[", mem))         return CompileList(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "{", mem))         return CompileTuple(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "{:", mem))        return CompileMap(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "or", mem))        return CompileOr(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "and", mem))       return CompileAnd(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "if", mem))        return CompileIf(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "do", mem))        return CompileDo(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "->", mem))        return CompileLambda(Tail(expr, mem), linkage, c);
  if (IsTagged(expr, "module", mem))    return CompileModule(Tail(expr, mem), linkage, c);

  if (IsTagged(expr, ".", mem))         return CompileOp(OpSeq(OpGet, mem), expr, linkage, c);
  if (IsTagged(expr, "not", mem))       return CompileOp(OpSeq(OpNot, mem), expr, linkage, c);
  if (IsTagged(expr, "#", mem))         return CompileOp(OpSeq(OpLen, mem), expr, linkage, c);
  if (IsTagged(expr, "*", mem))         return CompileOp(OpSeq(OpMul, mem), expr, linkage, c);
  if (IsTagged(expr, "/", mem))         return CompileOp(OpSeq(OpDiv, mem), expr, linkage, c);
  if (IsTagged(expr, "%", mem))         return CompileOp(OpSeq(OpRem, mem), expr, linkage, c);
  if (IsTagged(expr, "+", mem))         return CompileOp(OpSeq(OpAdd, mem), expr, linkage, c);
  if (IsTagged(expr, "-", mem)) {
    if (ListLength(expr, mem) == 2)     return CompileOp(OpSeq(OpNeg, mem), expr, linkage, c);
    else                                return CompileOp(OpSeq(OpSub, mem), expr, linkage, c);
  }
  if (IsTagged(expr, "|", mem))         return CompileOp(OpSeq(OpPair, mem), expr, linkage, c);
  if (IsTagged(expr, "in", mem))        return CompileOp(OpSeq(OpIn, mem), expr, linkage, c);
  if (IsTagged(expr, ">", mem))         return CompileOp(OpSeq(OpGt, mem), expr, linkage, c);
  if (IsTagged(expr, "<", mem))         return CompileOp(OpSeq(OpLt, mem), expr, linkage, c);
  if (IsTagged(expr, "==", mem))        return CompileOp(OpSeq(OpEq, mem), expr, linkage, c);

  if (IsPair(expr))                     return CompileApplication(expr, linkage, c);

  return CompileError("Unknown expression");
}

static CompileResult CompileConst(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  return CompileOk(
    EndWithLinkage(linkage,
      MakeSeq(0, 0,
        Pair(IntVal(OpConst),
        Pair(expr, nil, mem), mem)), mem));
}

static CompileResult CompileVar(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  Val pos = FindVar(expr, c->env, mem);
  if (IsNil(pos)) return CompileError("Undefined variable");

  return CompileOk(
    EndWithLinkage(linkage,
      MakeSeq(REnv, 0,
        Pair(IntVal(OpLookup),
        Pair(Head(pos, mem),
        Pair(Tail(pos, mem), nil, mem), mem), mem)), mem));
}

static CompileResult CompileList(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  Seq seq = MakeSeq(0, 0,
    Pair(IntVal(OpConst),
    Pair(nil, nil, mem), mem));

  while (!IsNil(expr)) {
    CompileResult item = CompileExpr(Head(expr, mem), LinkNext, c);
    if (!item.ok) return item;

    seq =
      Preserving(REnv, item.result,
      AppendSeq(
        MakeSeq(0, 0, Pair(IntVal(OpPair), nil, mem)),
        seq, mem), mem);

    expr = Tail(expr, mem);
  }

  return CompileOk(EndWithLinkage(linkage, seq, mem));
}

static CompileResult CompileTuple(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  u32 num_items = 0;
  Seq items_seq = EmptySeq();
  while (!IsNil(expr)) {
    CompileResult item = CompileExpr(Head(expr, mem), LinkNext, c);
    if (!item.ok) return item;

    items_seq = Preserving(REnv,
      AppendSeq(item.result,
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
          Pair(IntVal(OpTuple),
          Pair(IntVal(num_items), nil, mem), mem)),
        items_seq, mem), mem));
}

static CompileResult CompileMap(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

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
        AppendSeq(val.result,
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
      MakeSeq(0, 0, Pair(IntVal(OpMap), nil, mem)), mem), mem));
}

static CompileResult CompileItems(Val items, Compiler *c)
{
  Heap *mem = c->mem;

  Seq items_seq = EmptySeq();
  while (!IsNil(items)) {
    CompileResult item = CompileExpr(Head(items, mem), LinkNext, c);
    if (!item.ok) return item;

    items_seq = Preserving(REnv, item.result, items_seq, mem);
    items = Tail(items, mem);
  }

  return CompileOk(items_seq);
}

static CompileResult CompileOp(Seq op_seq, Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
  CompileResult args = CompileItems(ReverseList(Tail(expr, mem), mem), c);
  if (!args.ok) return args;

  return CompileOk(EndWithLinkage(linkage, AppendSeq(args.result, op_seq, mem), mem));
}

static CompileResult CompileOr(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

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
      a.result,
      EndWithLinkage(linkage,
        AppendSeq(
          MakeSeq(0, 0,
            Pair(IntVal(OpBranch),
            Pair(LabelRef(true_branch, mem),
            Pair(IntVal(OpPop), nil, mem), mem), mem)),
        AppendSeq(b.result,
          LabelSeq(after_b, mem), mem), mem), mem), mem));
}

static CompileResult CompileAnd(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  CompileResult a = CompileExpr(ListAt(expr, 0, mem), LinkNext, c);
  if (!a.ok) return a;

  Val true_branch = MakeLabel();
  Val after_b = MakeLabel();

  CompileResult b = CompileExpr(ListAt(expr, 1, mem), linkage, c);
  if (!b.ok) return b;

  Val a_linkage = Eq(linkage, LinkNext) ? after_b : linkage;

  return CompileOk(
    Preserving(REnv | RCont,
      a.result,
      AppendSeq(
        EndWithLinkage(a_linkage,
          MakeSeq(0, 0,
            Pair(IntVal(OpBranch),
            Pair(LabelRef(true_branch, mem), nil, mem), mem)), mem),
      AppendSeq(
        MakeSeq(0, 0,
          Pair(Label(true_branch, mem),
          Pair(IntVal(OpPop), nil, mem), mem)),
      AppendSeq(b.result,
        LabelSeq(after_b, mem), mem), mem), mem), mem));
}

static CompileResult CompileIf(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  CompileResult predicate = CompileExpr(ListAt(expr, 0, mem), LinkNext, c);
  if (!predicate.ok) return predicate;

  Val true_branch = MakeLabel();
  Val after = MakeLabel();

  Seq pred_seq =
    AppendSeq(predicate.result,
      MakeSeq(0, 0,
        Pair(IntVal(OpBranch),
        Pair(LabelRef(true_branch, mem), nil, mem), mem)), mem);

  Val alt_linkage = Eq(linkage, LinkNext) ? after : linkage;
  CompileResult alternative = CompileExpr(ListAt(expr, 2, mem), alt_linkage, c);
  if (!alternative.ok) return alternative;

  Seq alt_seq =
    AppendSeq(MakeSeq(0, 0, Pair(IntVal(OpPop), nil, mem)),
    alternative.result, mem);

  CompileResult consequent = CompileExpr(ListAt(expr, 1, mem), linkage, c);
  if (!consequent.ok) return consequent;

  Seq cons_seq =
    AppendSeq(MakeSeq(0, 0,
      Pair(Label(true_branch, mem),
      Pair(IntVal(OpPop), nil, mem), mem)),
    AppendSeq(consequent.result,
    LabelSeq(after, mem), mem), mem);

  return CompileOk(
    Preserving(REnv | RCont,
      pred_seq,
      ParallelSeq(alt_seq, cons_seq, mem), mem));
}

static CompileResult CompileAssigns(Val expr, u32 index, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  Seq assigns = EmptySeq();
  while (!IsNil(expr)) {
    Val assign = Head(expr, mem);
    Val var = Tail(ListAt(assign, 0, mem), mem);
    Define(index, var, c->env, mem);

    CompileResult value = CompileExpr(ListAt(assign, 1, mem), LinkNext, c);
    if (!value.ok) return value;

    assigns =
      AppendSeq(assigns,
        Preserving(REnv, value.result,
          MakeSeq(REnv, 0,
            Pair(IntVal(OpDefine),
            Pair(IntVal(index), nil, mem), mem)), mem), mem);

    index++;
    expr = Tail(expr, mem);
  }

  return CompileOk(
    EndWithLinkage(linkage,
    AppendSeq(assigns,
    MakeSeq(0, 0,
      Pair(IntVal(OpConst),
      Pair(MakeSymbol("ok", mem), nil, mem), mem)), mem), mem));
}

static CompileResult CompileImport(Val expr, u32 index, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

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
        Pair(IntVal(OpLoad),
        Pair(ModuleRef(mod, mem),
        Pair(IntVal(OpCont),
        Pair(LabelRef(after_load, mem),
        Pair(IntVal(OpApply),
        Pair(Label(after_load, mem), nil, mem), mem), mem), mem), mem), mem));

    assigns =
      AppendSeq(assigns,
      Preserving(REnv, load_seq,
      MakeSeq(REnv, 0,
        Pair(IntVal(OpDefine),
        Pair(IntVal(index), nil, mem), mem)), mem), mem);

    index++;
    expr = Tail(expr, mem);
  }

  return CompileOk(
    EndWithLinkage(linkage,
      AppendSeq(assigns,
        MakeSeq(0, 0,
          Pair(IntVal(OpConst),
          Pair(MakeSymbol("ok", mem), nil, mem), mem)), mem), mem));
}

static CompileResult CompileBlock(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
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

    Seq stmt_seq = stmt_res.result;
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
        Pair(IntVal(OpTuple),
        Pair(IntVal(index),
        Pair(IntVal(OpExtend), nil, mem), mem), mem)),
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
      Pair(IntVal(OpDefine), nil, mem), mem), mem), mem));
  } else {
    u32 num_params = ListLength(params, mem);
    c->env = ExtendEnv(c->env, MakeTuple(num_params, mem), mem);
    for (u32 i = 0; i < num_params; i++) {
      Val param = Tail(Head(params, mem), mem);
      Define(i, param, c->env, mem);
      params = Tail(params, mem);
    }

    params_code = MakeSeq(0, 0, Pair(IntVal(OpExtend), nil, mem));
  }

  CompileResult body_code = CompileExpr(body, LinkReturn, c);
  if (!body_code.ok) return body_code;

  c->env = RestoreEnv(c->env, mem);

  Val after_lambda = MakeLabel();

  return CompileOk(
    EndWithLinkage(linkage,
    TackOnSeq(
      MakeSeq(0, 0,
        Pair(IntVal(OpLambda),
        Pair(LabelRef(after_lambda, mem), nil, mem), mem)),
    AppendSeq(params_code,
    AppendSeq(body_code.result,
    LabelSeq(after_lambda, mem), mem), mem), mem), mem));
}

static CompileResult CompileModule(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;
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
      Pair(IntVal(OpConst),
      Pair(TupleGet(frame, i, mem),
      Pair(IntVal(OpSet),
      Pair(IntVal(i), nil, mem), mem), mem), mem));
    keys_seq = AppendSeq(keys_seq, key_seq, mem);
  }

  Seq mod_body = EndWithLinkage(LinkReturn,
    AppendSeq(block.result,
    AppendSeq(keys_seq,
    MakeSeq(0, 0,
      Pair(IntVal(OpMap),
      Pair(IntVal(OpDup),
      Pair(IntVal(OpModule),
      Pair(ModuleRef(name, mem), nil, mem), mem), mem), mem)), mem), mem), mem);

  return CompileOk(
    EndWithLinkage(linkage,
    AppendSeq(
      TackOnSeq(MakeSeq(0, 0,
        Pair(IntVal(OpLambda),
        Pair(LabelRef(after, mem), nil, mem), mem)),
      mod_body, mem),
    MakeSeq(0, 0,
      Pair(Label(after, mem),
      Pair(IntVal(OpModule),
      Pair(ModuleDef(name, mem), nil, mem), mem), mem)), mem), mem));
}

static CompileResult CompileApplication(Val expr, Linkage linkage, Compiler *c)
{
  Heap *mem = c->mem;

  CompileResult op = CompileExpr(Head(expr, mem), LinkNext, c);
  if (!op.ok) return op;

  CompileResult args = CompileTuple(Tail(expr, mem), LinkNext, c);
  if (!args.ok) return args;

  Seq call = Preserving(REnv, args.result, op.result, mem);

  if (Eq(linkage, LinkReturn)) {
    return CompileOk(
      Preserving(RCont,
        call,
        MakeSeq(RCont, REnv, Pair(IntVal(OpApply), nil, mem)), mem));
  }

  Val after_call = MakeLabel();
  if (Eq(linkage, LinkNext)) linkage = after_call;

  return CompileOk(
    AppendSeq(call,
    MakeSeq(0, REnv | RCont,
      Pair(IntVal(OpCont),
      Pair(LabelRef(linkage, mem),
      Pair(IntVal(OpApply),
      Pair(Label(after_call, mem), nil, mem), mem), mem), mem)), mem));
}
