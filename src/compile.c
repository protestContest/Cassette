#include "compile.h"
#include "ast.h"
#include "ops.h"
#include "env.h"
#include "print.h"

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkNext    SymbolFor("next")

#define RVal    (1 << RegVal)
#define REnv    (1 << RegEnv)
#define RCon    (1 << RegCont)
#define RFun    (1 << RegFun)
#define RArgs   (1 << RegArgs)
#define RArg1   (1 << RegArg1)
#define RArg2   (1 << RegArg2)
#define RStack  (1 << RegStack)
#define RAll    (RVal | REnv | RCon | RFun | RArgs | RArg1 | RArg2 | RStack)

#if DEBUG_COMPILE
#define DebugMsg(x)   Print(x)
#else
#define DebugMsg(x)   (void)0
#endif

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

typedef struct {
  Mem *mem;
  Val env;
} Compiler;

#define MakeSeq(needs, modifies, stmts)   (Seq){true, needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))

static Seq CompileExp(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileDefinitions(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileAnd(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileOr(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileAccess(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileBlock(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileLambdaBody(Val exp, Val label, Compiler *c);
static Seq CompileImport(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileUnaryOp(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileBinaryOp(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileBinary(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompilePrimitiveCall(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileArguments(Val exp, Reg target, Compiler *c);
static Seq CompileCall(Reg target, Linkage linkage, Compiler *c);

static Seq CompileLinkage(Linkage linkage, Compiler *c);
static Seq EndWithLinkage(Linkage linkage, Seq seq, Compiler *c);
static Seq AppendSeq(Seq seq1, Seq seq2, Compiler *c);
static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Compiler *c);
static Seq TackOn(Seq seq1, Seq seq2, Compiler *c);
static Seq ParallelSeq(Seq seq1, Seq seq2, Compiler *c);
static Val MakeLabel(Compiler *c);
static Val Label(Val label, Compiler *c);
static Val LabelRef(Val label, Compiler *c);
static Seq LabelSeq(Val label, Compiler *c);
static Val RegRef(Reg reg, Compiler *c);
Val ExtractLabels(Val stmts, Compiler *c);
Seq CompileError(char *message, Val val, Compiler *c);
static void PrintSeq(Seq seq, Mem *mem);
static bool IsSelfEvaluating(Val exp);
static bool IsUnaryOp(Val exp, Mem *mem);
static bool IsBinaryOp(Val exp, Mem *mem);
static Val PrimitiveOp(Val name);

void InitCompiler(Compiler *c, Mem *mem)
{
  c->mem = mem;
  c->env = InitialEnv(mem);

  // symbols for opcodes
  InitOps(mem);

  // next linkage
  MakeSymbol(mem, "next");

  // AST tags
  MakeSymbol(mem, ":");
  MakeSymbol(mem, "@");
  MakeSymbol(mem, "let");
  MakeSymbol(mem, "if");
  MakeSymbol(mem, "do");
  MakeSymbol(mem, "->");
  MakeSymbol(mem, "import");

  // utility tags
  MakeSymbol(mem, "label");
  MakeSymbol(mem, "label-ref");
  MakeSymbol(mem, "reg");
}

Val Compile(Val exp, Mem *mem)
{
  Compiler c;
  InitCompiler(&c, mem);

  Seq compiled = AppendSeq(
    CompileExp(exp, RVal, LinkNext, &c),
    MakeSeq(0, 0, MakePair(mem, OpSymbol(OpHalt), nil)), &c);

  return ExtractLabels(compiled.stmts, &c);
}

static Seq CompileExp(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

#if DEBUG_COMPILE
  Print("Compiling ");
  PrintVal(mem, exp);
  Print("\n");
#endif

  Seq result;

  if (IsSelfEvaluating(exp)) {
    result = CompileSelf(exp, target, linkage, c);
    DebugMsg("[self] ");
  } else if (IsSym(exp)) {
    result = CompileVariable(exp, target, linkage, c);
    DebugMsg("[var] ");
  } else if (IsBinary(mem, exp)) {
    result = CompileBinary(exp, target, linkage, c);
    DebugMsg("[binary] ");
  } else if (IsTagged(mem, exp, ":")) {
    result = CompileSelf(Second(mem, exp), target, linkage, c);
    DebugMsg("[symbol] ");
  } else if (IsTagged(mem, exp, "[")) {
    result = CompileSelf(Tail(mem, exp), target, linkage, c);
    DebugMsg("[list] ");
  } else if (IsTagged(mem, exp, "let")) {
    result = CompileDefinitions(Tail(mem, exp), target, linkage, c);
    DebugMsg("[let] ");
  } else if (IsTagged(mem, exp, "if")) {
    result = CompileIf(Tail(mem, exp), target, linkage, c);
    DebugMsg("[if] ");
  } else if (IsTagged(mem, exp, "and")) {
    result = CompileAnd(Tail(mem, exp), target, linkage, c);
    DebugMsg("[and] ");
  } else if (IsTagged(mem, exp, "or")) {
    result = CompileOr(Tail(mem, exp), target, linkage, c);
    DebugMsg("[or] ");
  } else if (IsTagged(mem, exp, "do")) {
    result = CompileBlock(Tail(mem, exp), target, linkage, c);
    DebugMsg("[do] ");
  } else if (IsTagged(mem, exp, "->")) {
    result = CompileLambda(Tail(mem, exp), target, linkage, c);
    DebugMsg("[lambda] ");
  } else if (IsTagged(mem, exp, "import")) {
    result = CompileImport(Tail(mem, exp), target, linkage, c);
    DebugMsg("[import] ");
  } else if (IsUnaryOp(exp, mem)) {
    result = CompileUnaryOp(exp, target, linkage, c);
    DebugMsg("[unary] ");
  } else if (IsBinaryOp(exp, mem)) {
    result = CompileBinaryOp(exp, target, linkage, c);
    DebugMsg("[binary] ");
  } else if (IsTagged(mem, exp, "@")) {
    result = CompilePrimitiveCall(Tail(mem, exp), target, linkage, c);
    DebugMsg("[primitive] ");
  } else {
    result = CompileApplication(exp, target, linkage, c);
    DebugMsg("[call] ");
  }

#if DEBUG_COMPILE
  PrintVal(mem, exp);
  Print("\n");
  PrintSeq(result, mem);
  Print("───────────────────────────────────────────────────\n");
#endif

  return result;
}

static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  return
    EndWithLinkage(linkage,
      MakeSeq(0, target,
        MakePair(mem, OpSymbol(OpConst),
        MakePair(mem, exp,
        MakePair(mem, RegRef(target, c), nil)))), c);
}

static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val var_pos = LookupPosition(exp, c->env, c->mem);

  if (IsNil(var_pos)) {
    return CompileError("Undefined variable", exp, c);
  } else {
    return
      EndWithLinkage(linkage,
        MakeSeq(REnv, target,
          MakePair(mem, OpSymbol(OpLookup),
          MakePair(mem, Head(mem, var_pos),
          MakePair(mem, Tail(mem, var_pos),
          MakePair(mem, RegRef(target, c), nil))))), c);
  }
}

static Seq CompileDefinitions(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  Val var = First(mem, exp);

  Seq val_code = CompileExp(Second(mem, exp), RVal, LinkNext, c);
  // Define(var, nil, c->env, c->mem);
  Seq def_code = MakeSeq(REnv | RVal, 0,
      MakePair(mem, OpSymbol(OpDefine),
      MakePair(mem, var,
      MakePair(mem, RegRef(RVal, c), nil))));

  Val rest = ListFrom(mem, exp, 2);

  if (IsNil(rest)) {
    return
      EndWithLinkage(linkage,
        Preserving(REnv, val_code, def_code, c), c);
  } else {
    return
      Preserving(REnv | RCon,
        Preserving(REnv, val_code, def_code, c),
        CompileDefinitions(rest, target, linkage, c), c);
  }
}

static Seq CompileIf(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val true_label = MakeLabel(c);
  Val after_label = MakeLabel(c);
  Linkage false_linkage = (Eq(linkage, LinkNext)) ? after_label : linkage;

  Seq predicate_code = CompileExp(First(mem, exp), RVal, LinkNext, c);
  Seq true_code = CompileExp(Second(mem, exp), target, linkage, c);
  Seq false_code = CompileExp(Third(mem, exp), target, false_linkage, c);

  Seq test_code =
    MakeSeq(RVal, 0,
      MakePair(mem, OpSymbol(OpBranch),
      MakePair(mem, LabelRef(true_label, c),
      MakePair(mem, RegRef(RVal, c),
              nil))));

  Seq branches = ParallelSeq(
    false_code,
    AppendSeq(LabelSeq(true_label, c), true_code, c), c);

  return
    Preserving(REnv | RCon,
      predicate_code,
      AppendSeq(test_code,
      AppendSeq(branches,
                LabelSeq(after_label, c), c), c), c);
}

static Seq CompileAnd(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val after_label = MakeLabel(c);

  Seq a_code = CompileExp(First(mem, exp), target, LinkNext, c);
  Seq b_code = CompileExp(Second(mem, exp), target, LinkNext, c);

  Seq test_code = MakeSeq(target, 0,
    MakePair(mem, OpSymbol(OpBranchF),
    MakePair(mem, LabelRef(after_label, c),
    MakePair(mem, RegRef(target, c),
            nil))));

  return
    EndWithLinkage(linkage,
      Preserving(REnv | RCon,
        a_code,
        AppendSeq(test_code,
        AppendSeq(b_code, LabelSeq(after_label, c), c), c), c), c);
}

static Seq CompileOr(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val after_label = MakeLabel(c);

  Seq a_code = CompileExp(First(mem, exp), RVal, LinkNext, c);
  Seq b_code = CompileExp(Second(mem, exp), RVal, LinkNext, c);

  Seq test_code = MakeSeq(target, 0,
    MakePair(mem, OpSymbol(OpBranch),
    MakePair(mem, LabelRef(after_label, c),
    MakePair(mem, RegRef(target, c),
            nil))));

  return
    EndWithLinkage(linkage,
      Preserving(REnv | RCon,
        a_code,
        AppendSeq(test_code,
        AppendSeq(b_code, LabelSeq(after_label, c), c), c), c), c);
}

static Seq CompileBlock(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  Val stmts = exp;
  while (!IsNil(stmts)) {
    Val stmt = Head(mem, stmts);
    if (IsTagged(mem, stmt, "let")) {
      Val defs = Tail(mem, stmt);
      while (!IsNil(defs)) {
        Val var = Head(mem, defs);
        Define(var, nil, c->env, mem);
        defs = ListFrom(mem, defs, 2);
      }
    }
    stmts = Tail(mem, stmts);
  }

  // scan through block and define vars in the correct place
  // Val stmts = exp;
  // while (!IsNil(stmts)) {
  //   Val stmt = Head(c->mem, stmts);
  //   if (IsTagged(c->mem, stmt, "let")) {
  //     Val defines = Tail(c->mem, stmt);
  //     while (!IsNil(defines)) {
  //       Val var = Head(c->mem, defines);
  //       Define(var, nil, c->env, c->mem);
  //       defines = ListFrom(c->mem, defines, 2);
  //     }
  //   }
  //   stmts = Tail(c->mem, stmts);
  // }

  return CompileSequence(exp, target, linkage, c);
}

static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  if (IsNil(Tail(mem, exp))) {
    return CompileExp(Head(mem, exp), target, linkage, c);
  }

  return Preserving(REnv | RCon,
                    CompileExp(Head(mem, exp), target, LinkNext, c),
                    CompileSequence(Tail(mem, exp), target, linkage, c), c);
}

static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val proc_label = MakeLabel(c);
  Val after_label = MakeLabel(c);
  Linkage lambda_linkage = (Eq(linkage, LinkNext)) ? after_label : linkage;

  // pushes the list of [env], proc_label onto the target
  Seq lambda_code =
    EndWithLinkage(lambda_linkage,
      MakeSeq(REnv, target,
        MakePair(mem, OpSymbol(OpLambda),
        MakePair(mem, LabelRef(proc_label, c),
        MakePair(mem, RegRef(target, c),
                  nil)))), c);

  Seq lambda_body =
    AppendSeq(CompileLambdaBody(exp, proc_label, c), LabelSeq(after_label, c), c);

  return TackOn(lambda_code, lambda_body, c);
}

static Seq CompileLambdaBody(Val exp, Val proc_label, Compiler *c)
{
  Mem *mem = c->mem;

  Val params = First(mem, exp);

  // extend environment
  c->env = MakePair(mem, nil, c->env);
  Seq env_code =
    MakeSeq(REnv, REnv,
      MakePair(mem, OpSymbol(OpPair),
      MakePair(mem, nil,
      MakePair(mem, RegRef(REnv, c), nil))));

  // define parameters
  Seq bind_code = EmptySeq(mem);
  while (!IsNil(params)) {
    Val param = Head(mem, params);
    Define(param, nil, c->env, c->mem);

    // pop an argument into val, and define it
    bind_code = AppendSeq(
      bind_code,
      MakeSeq(REnv | RArgs, RVal | RArgs,
        MakePair(mem, OpSymbol(OpPop),
        MakePair(mem, RegRef(RArgs, c),
        MakePair(mem, RegRef(RVal, c),
        MakePair(mem, OpSymbol(OpDefine),
        MakePair(mem, param,
        MakePair(mem, RegRef(RVal, c), nil))))))), c);

    params = Tail(mem, params);
  }

  Seq body_code = CompileExp(Second(mem, exp), RVal, LinkReturn, c);

  c->env = Tail(mem, c->env);

  return
    AppendSeq(LabelSeq(proc_label, c),
    AppendSeq(env_code,
    AppendSeq(bind_code, body_code, c), c), c);
}

static Seq CompileImport(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  return CompileError("Imports not yet supported", nil, c);
}

static Seq CompileUnaryOp(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Seq arg = CompileExp(Second(mem, exp), target, LinkNext, c);

  Seq op_seq =
    MakeSeq(target, target,
      MakePair(mem, PrimitiveOp(First(mem, exp)),
      MakePair(mem, RegRef(target, c), nil)));

  return
    EndWithLinkage(linkage,
      AppendSeq(arg, op_seq, c), c);
}

static Seq CompileBinaryOp(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Seq arg1 = CompileExp(Second(mem, exp), RArg1, LinkNext, c);
  Seq arg2 = CompileExp(Third(mem, exp), RArg2, LinkNext, c);

  Seq op_seq;
  if (IsTagged(mem, exp, ">=")) {
    op_seq =
      MakeSeq(RArg1 | RArg2, target,
        MakePair(mem, OpSymbol(OpLt),
        MakePair(mem, RegRef(target, c),
        MakePair(mem, OpSymbol(OpNot),
        MakePair(mem, RegRef(target, c), nil)))));
  } else if (IsTagged(mem, exp, "<=")) {
    op_seq =
      MakeSeq(RArg1 | RArg2, target,
        MakePair(mem, OpSymbol(OpGt),
        MakePair(mem, RegRef(target, c),
        MakePair(mem, OpSymbol(OpNot),
        MakePair(mem, RegRef(target, c), nil)))));
  } else {
    op_seq =
      MakeSeq(RArg1 | RArg2, target,
        MakePair(mem, PrimitiveOp(First(mem, exp)),
        MakePair(mem, RegRef(target, c), nil)));
  }

  return
    EndWithLinkage(linkage,
      Preserving(REnv,
        arg1,
        Preserving(RArg1,
          arg2,
          op_seq, c), c), c);
}

static Seq CompileBinary(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  Val sym = MakeSymbolFrom(mem, (char*)BinaryData(mem, exp), BinaryLength(mem, exp));

  return
    EndWithLinkage(linkage,
      MakeSeq(0, target,
        MakePair(mem, OpSymbol(OpStr),
        MakePair(mem, sym,
        MakePair(mem, RegRef(target, c),
                nil)))), c);
}

static Seq CompilePrimitiveCall(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val op_name = First(mem, exp);
  Seq args_code = CompileArguments(Tail(mem, exp), RArgs, c);

  Seq call_code =
    MakeSeq(RArgs, RFun | target,
      MakePair(mem, OpSymbol(OpConst),
      MakePair(mem, op_name,
      MakePair(mem, RegRef(RFun, c),
      MakePair(mem, OpSymbol(OpPrim),
      MakePair(mem, RegRef(target, c), nil))))));

  return EndWithLinkage(linkage, AppendSeq(args_code, call_code, c), c);
}

static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  Seq args_code = CompileArguments(Tail(mem, exp), RArgs, c);

  // compile the operator into RFun, then put the proc's env in REnv and the
  // code location in RFun
  Seq proc_code =
    AppendSeq(
      CompileExp(Head(mem, exp), RFun, LinkNext, c),
      MakeSeq(RFun, RFun | REnv,
        MakePair(mem, OpSymbol(OpPop),
        MakePair(mem, RegRef(RFun, c),
        MakePair(mem, RegRef(REnv, c),
                  nil)))), c);

  Seq call_code = CompileCall(target, linkage, c);

  return
    Preserving(RCon | REnv,
      args_code,
      Preserving(RArgs | RCon,
        proc_code,
        call_code, c), c);
}

static Seq CompileArguments(Val exp, Reg target, Compiler *c)
{
  Mem *mem = c->mem;

  if (IsNil(exp)) {
    return EmptySeq();
  }

  Val args = ReverseOnto(mem, exp, nil);
  Seq first_arg_code = CompileExp(Head(mem, args), RVal, LinkNext, c);
  args = Tail(mem, args);

  // initialize RArgs with empty list, push first arg
  Seq init_code = MakeSeq(RVal, target,
    MakePair(mem, OpSymbol(OpConst),
    MakePair(mem, nil,
    MakePair(mem, RegRef(target, c),
    MakePair(mem, OpSymbol(OpPush),
    MakePair(mem, RegRef(RVal, c),
    MakePair(mem, RegRef(target, c), nil)))))));

  Seq args_code = AppendSeq(first_arg_code, init_code, c);

  // compile each arg into RVal, then push it onto RArgs list
  while (!IsNil(args)) {
    Seq arg_code = CompileExp(Head(mem, args), RVal, LinkNext, c);

    Seq push_code =
      MakeSeq(RVal | target, target,
        MakePair(mem, OpSymbol(OpPush),
        MakePair(mem, RegRef(RVal, c),
        MakePair(mem, RegRef(target, c), nil))));

    // last arg doesn't need to preserve REnv
    Reg preserve = target;
    if (!IsNil(Tail(mem, args))) preserve = target | REnv;

    args_code =
      AppendSeq(args_code,
        Preserving(preserve, arg_code, push_code, c), c);

    args = Tail(mem, args);
  }

  return args_code;
}

static Seq CompileCall(Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

  if (target != RVal) {
    Assert(!Eq(linkage, LinkReturn));
    Val proc_return = MakeLabel(c);

    return
      EndWithLinkage(linkage,
        MakeSeq(RFun | RArgs, RAll,
          // set RCon to label after the call
          MakePair(mem, OpSymbol(OpConst),
          MakePair(mem, LabelRef(proc_return, c),
          MakePair(mem, RegRef(RCon, c),
          // jump to function body
          MakePair(mem, OpSymbol(OpGoto),
          MakePair(mem, RegRef(RFun, c),
          MakePair(mem, Label(proc_return, c),
          // move result to target
          MakePair(mem, OpSymbol(OpMove),
          MakePair(mem, RegRef(RVal, c),
          MakePair(mem, RegRef(target, c), nil)))))))))), c);
  }

  // tail recursive call — RCon already set up from previous
  if (Eq(linkage, LinkReturn)) {
    return
      MakeSeq(RFun | RCon | RArgs, RAll,
        MakePair(mem, OpSymbol(OpGoto),
        MakePair(mem, RegRef(RFun, c), nil)));
  }

  Val proc_return = MakeLabel(c);
  if (Eq(linkage, LinkNext)) linkage = proc_return;

  return
    MakeSeq(RFun | RArgs, RAll,
      MakePair(mem, OpSymbol(OpConst),
      MakePair(mem, LabelRef(linkage, c),
      MakePair(mem, RegRef(RCon, c),
      MakePair(mem, OpSymbol(OpGoto),
      MakePair(mem, RegRef(RFun, c),
      MakePair(mem, Label(proc_return, c), nil)))))));
}

static Seq CompileLinkage(Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RCon, 0,
      MakePair(mem, OpSymbol(OpGoto),
      MakePair(mem, RegRef(RCon, c), nil)));
  }

  if (Eq(linkage, LinkNext)) {
    return EmptySeq(mem);
  }

  return MakeSeq(0, 0,
    MakePair(mem, OpSymbol(OpJump),
    MakePair(mem, LabelRef(linkage, c), nil)));
}

static Seq EndWithLinkage(Linkage linkage, Seq seq, Compiler *c)
{
  if (!seq.ok) return seq;
  return Preserving(RCon, seq, CompileLinkage(linkage, c), c);
}

static Seq AppendSeq(Seq seq1, Seq seq2, Compiler *c)
{
  Mem *mem = c->mem;
  if (!seq1.ok) return seq1;
  if (!seq2.ok) return seq2;

  if (IsEmptySeq(seq1)) return seq2;
  if (IsEmptySeq(seq2)) return seq1;

  Reg needs = seq1.needs | (seq2.needs & ~seq1.modifies);
  Reg modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Compiler *c)
{
  Mem *mem = c->mem;
  if (!seq1.ok) return seq1;
  if (!seq2.ok) return seq2;

  if (regs == 0) {
    return AppendSeq(seq1, seq2, c);
  }

  Reg reg = 0;
  for (u32 i = 0; i < 7; i++) {
    if (regs & Bit(i)) {
      reg = Bit(i);
      break;
    }
  }

  regs = regs & ~reg;
  if ((seq2.needs & reg) && (seq1.modifies & reg)) {
    Reg needs = reg | seq1.needs;
    Reg modifies = seq1.modifies & ~reg;

    return
      Preserving(regs,
        MakeSeq(needs, modifies,
          ListConcat(mem,
            MakePair(mem, OpSymbol(OpPush),
            MakePair(mem, RegRef(reg, c),
            MakePair(mem, RegRef(RStack, c), seq1.stmts))),
            MakePair(mem, OpSymbol(OpPop),
            MakePair(mem, RegRef(RStack, c),
            MakePair(mem, RegRef(reg, c), nil))))),
        seq2, c);
  } else {
    return Preserving(regs, seq1, seq2, c);
  }
}

static Seq TackOn(Seq seq1, Seq seq2, Compiler *c)
{
  Mem *mem = c->mem;
  if (!seq1.ok) return seq1;
  if (!seq2.ok) return seq2;

  return MakeSeq(seq1.needs, seq1.modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static Seq ParallelSeq(Seq seq1, Seq seq2, Compiler *c)
{
  Mem *mem = c->mem;
  if (!seq1.ok) return seq1;
  if (!seq2.ok) return seq2;

  Reg needs = seq1.needs | seq2.needs;
  Reg modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static i32 label_num = 0;
static Val MakeLabel(Compiler *c)
{
  Mem *mem = c->mem;
  char label[12] = "lbl";
  u32 len = IntToString(label_num++, label+3, 7);
  label[len+3] = 0;
  return MakeSymbol(mem, label);
}

static Val Label(Val label, Compiler *c)
{
  return MakePair(c->mem, SymbolFor("label"), label);
}

static Val LabelRef(Val label, Compiler *c)
{
  Mem *mem = c->mem;
  return MakePair(mem, SymbolFor("label-ref"), label);
}

static Seq LabelSeq(Val label, Compiler *c)
{
  Mem *mem = c->mem;
  return MakeSeq(0, 0,
    MakePair(mem, Label(label, c), nil));
}

static Val RegRef(u32 reg_flag, Compiler *c)
{
  Mem *mem = c->mem;
  Assert(reg_flag != 0);
  Reg reg = 0;
  while ((reg_flag & 1) == 0) {
    reg++;
    reg_flag >>= 1;
  }
  Assert((reg_flag >> 1) == 0);
  return MakePair(mem, SymbolFor("reg"), IntVal(reg));
}

static void PrintStmtArg(Val arg, Mem *mem)
{
  if (IsSym(arg)) {
    PrintSymbol(mem, arg);
  } else if (IsTagged(mem, arg, "label-ref")) {
    Print(":");
    PrintSymbol(mem, Tail(mem, arg));
  } else if (IsTagged(mem, arg, "reg")) {
    i32 reg = RawInt(Tail(mem, arg));
    PrintReg(reg);
  } else {
    PrintVal(mem, arg);
  }
}

static u32 PrintStmt(Val stmts, Mem *mem)
{
  OpCode op = OpForSymbol(First(mem, stmts));
  PrintOpCode(op);
  stmts = Tail(mem, stmts);

  for (u32 i = 0; i < OpLength(op) - 1; i++) {
    Print(" ");
    PrintStmtArg(Head(mem, stmts), mem);
    stmts = Tail(mem, stmts);
  }
  Print("\n");
  return OpLength(op);
}

void PrintStmts(Val stmts, Mem *mem)
{
  i32 i = 0;
  while (!IsNil(stmts)) {
    Val stmt = Head(mem, stmts);

    if (IsTagged(mem, stmt, "label")) {
      PrintSymbol(mem, Tail(mem, stmt));
      Print("\n");
      i++;
      stmts = Tail(mem, stmts);
    } else {
      PrintIntN(i, 4, ' ');
      Print("│ ");

      u32 len = PrintStmt(stmts, mem);
      i += len;
      stmts = ListFrom(mem, stmts, len);
    }
  }
}

static void PrintSeq(Seq seq, Mem *mem)
{
  Print("Needs: ");
  if (seq.needs == 0) {
    Print("[] ");
  } else {
    i32 reg = 0;
    while (seq.needs != 0) {
      if (seq.needs & 1) {
        PrintReg(reg);
        Print(" ");
      }
      seq.needs >>= 1;
      reg++;
    }
  }

  Print("Modifies: ");
  if (seq.modifies == 0) {
    Print("[]");
  } else {
    i32 reg = 0;
    while (seq.modifies != 0) {
      if (seq.modifies & 1) {
        PrintReg(reg);
        Print(" ");
      }
      seq.modifies >>= 1;
      reg++;
    }
  }
  Print("\n");
  PrintStmts(seq.stmts, mem);
}

Val ReplaceLabels(Val stmts, Map labels, Compiler *c)
{
  Mem *mem = c->mem;
  if (IsNil(stmts)) return nil;

  Val stmt = Head(mem, stmts);
  if (IsTagged(mem, stmt, "label")) {
    return ReplaceLabels(Tail(mem, stmts), labels, c);
  }

  if (IsTagged(mem, stmt, "label-ref")) {
    Val label = Tail(mem, stmt);
    Assert(MapContains(&labels, label.as_v));
    u32 pos = MapGet(&labels, label.as_v);
    return MakePair(mem, IntVal(pos), ReplaceLabels(Tail(mem, stmts), labels, c));
  }

  return MakePair(mem, stmt, ReplaceLabels(Tail(mem, stmts), labels, c));
}

Val ExtractLabels(Val stmts, Compiler *c)
{
  Mem *mem = c->mem;
  Map labels;
  InitMap(&labels);

  u32 i = 0;
  Val cur = stmts;
  while (!IsNil(cur)) {
    Val stmt = Head(mem, cur);
    if (IsTagged(mem, stmt, "label")) {
      Val label = Tail(mem, stmt);
      MapSet(&labels, label.as_v, i);
    } else {
      i++;
    }
    cur = Tail(mem, cur);
  }

  return ReplaceLabels(stmts, labels, c);
}

Seq CompileError(char *message, Val val, Compiler *c)
{
  Mem *mem = c->mem;
  PrintEscape(IOFGRed);
  Print("(Compile Error) ");
  Print(message);

  if (!IsNil(val)) {
    Print(": ");
    PrintVal(mem, val);
  }
  Print("\n");
  PrintEscape(IOFGReset);
  return (Seq){false, 0, 0, nil};
}

static bool IsSelfEvaluating(Val exp)
{
  return
    IsNil(exp) ||
    Eq(exp, SymbolFor("true")) ||
    Eq(exp, SymbolFor("false")) ||
    IsNumeric(exp);
}

static bool IsUnaryOp(Val exp, Mem *mem)
{
  return
    IsTagged(mem, exp, "not") ||
    IsTagged(mem, exp, "\"");
}

static bool IsBinaryOp(Val exp, Mem *mem)
{
  return
    IsTagged(mem, exp, "in") ||
    IsTagged(mem, exp, "==") ||
    IsTagged(mem, exp, "!=") ||
    IsTagged(mem, exp, ">=") ||
    IsTagged(mem, exp, "<=") ||
    IsTagged(mem, exp, ">") ||
    IsTagged(mem, exp, "<") ||
    IsTagged(mem, exp, "+") ||
    IsTagged(mem, exp, "-") ||
    IsTagged(mem, exp, "*") ||
    IsTagged(mem, exp, "/");
}

static Val PrimitiveOp(Val name)
{
  if (Eq(name, SymbolFor("==")))  return OpSymbol(OpEqual);
  if (Eq(name, SymbolFor(">")))   return OpSymbol(OpGt);
  if (Eq(name, SymbolFor("<")))   return OpSymbol(OpLt);
  if (Eq(name, SymbolFor("+")))   return OpSymbol(OpAdd);
  if (Eq(name, SymbolFor("-")))   return OpSymbol(OpSub);
  if (Eq(name, SymbolFor("*")))   return OpSymbol(OpMul);
  if (Eq(name, SymbolFor("/")))   return OpSymbol(OpDiv);
  return name;
}
