#include "compile.h"
#include "ast.h"
#include "ops.h"

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkNext    SymbolFor("next")

#define RVal  (1 << RegVal)
#define REnv  (1 << RegEnv)
#define RCon  (1 << RegCon)
#define RFun  (1 << RegFun)
#define RArg  (1 << RegArg)
#define RAll  (RVal | REnv | RCon | RFun | RArg)

typedef struct {
  bool ok;
  u32 needs;
  u32 modifies;
  Val stmts;
} Seq;

typedef struct {
  Mem *mem;
} Compiler;

#define MakeSeq(needs, modifies, stmts)   (Seq){true, needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))

static Seq CompileExp(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileDefinitions(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileDefinition(Val var, Val val, Reg target, Linkage linkage, Compiler *c);
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileAnd(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileOr(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileAccess(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileLambdaBody(Val exp, Val label, Compiler *c);
static Seq CompileImport(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Compiler *c);
static Seq CompileArgs(Val exp, Compiler *c);
static Seq CompileCall(Reg target, Linkage linkage, Compiler *c);
static Seq CompileProcAppl(Reg target, Linkage linkage, Compiler *c);

static Seq CompileLinkage(Linkage linkage, Compiler *c);
static Seq EndWithLinkage(Linkage linkage, Seq seq, Compiler *c);
static Seq AppendSeq(Seq seq1, Seq seq2, Compiler *c);
static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Compiler *c);
static Seq TackOn(Seq seq1, Seq seq2, Compiler *c);
static Seq ParallelSeq(Seq seq1, Seq seq2, Compiler *c);
static Val MakeLabel(Compiler *c);
static Val LabelRef(Val label, Compiler *c);
static Seq LabelSeq(Val label, Compiler *c);
static Val RegRef(Reg reg, Compiler *c);
Val ExtractLabels(Val stmts, Compiler *c);
Seq CompileError(char *message, Val val, Compiler *c);

void InitCompiler(Compiler *c, Mem *mem)
{
  c->mem = mem;

  // symbols for opcodes
  InitOps(mem);

  // next linkage
  MakeSymbol(mem, "next");

  // AST tags
  MakeSymbol(mem, ":");
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
  Seq compiled = CompileExp(exp, RVal, LinkNext, &c);

#if DEBUG_COMPILE
  Print("\n");
#endif

  return ExtractLabels(compiled.stmts, &c);
}

#if DEBUG_COMPILE
static u32 indent = 0;
#endif
static Seq CompileExp(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;

#if DEBUG_COMPILE
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Compile: ");
  PrintVal(mem, exp);
  indent++;
#endif

  Seq result;
  if (IsNil(exp) || IsNumeric(exp)) {
#if DEBUG_COMPILE
    Print(" (Self)");
#endif
    result = CompileSelf(exp, target, linkage, c);
  } else if (IsTagged(mem, exp, ":")) {
#if DEBUG_COMPILE
    Print(" (Quote)");
#endif
    result = CompileSelf(ListAt(mem, exp, 1), target, linkage, c);
  } else if (IsSym(exp)) {
#if DEBUG_COMPILE
    Print(" (Variable)");
#endif
    result = CompileVariable(exp, target, linkage, c);
  } else if (IsTagged(mem, exp, "let")) {
#if DEBUG_COMPILE
    Print(" (Definition)");
#endif
    result = CompileDefinitions(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "if")) {
#if DEBUG_COMPILE
    Print(" (If)");
#endif
    result = CompileIf(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "and")) {
#if DEBUG_COMPILE
    Print(" (And)");
#endif
    result = CompileAnd(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "or")) {
#if DEBUG_COMPILE
    Print(" (Or)");
#endif
    result = CompileOr(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "do")) {
#if DEBUG_COMPILE
    Print(" (Sequence)");
#endif
    result = CompileSequence(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "->")) {
#if DEBUG_COMPILE
    Print(" (Lambda)");
#endif
    result = CompileLambda(Tail(mem, exp), target, linkage, c);
  } else if (IsTagged(mem, exp, "import")) {
#if DEBUG_COMPILE
    Print(" (Import)");
#endif
    result = CompileImport(Tail(mem, exp), target, linkage, c);
  } else {
#if DEBUG_COMPILE
    Print(" (Application)");
#endif
    result = CompileApplication(exp, target, linkage, c);
  }

#if DEBUG_COMPILE
  indent--;
  Print("\n");
  PrintVal(mem, exp);
  Print(" => \n");
  PrintSeq(result.stmts, mem);
  Print("───────────────────────────");
#endif

  return result;
}

/*
The `const` op has two parameters, a value and a register. It puts the value in
the register.
*/
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  return
    EndWithLinkage(linkage,
      MakeSeq(0, target,
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, exp,
        MakePair(mem, RegRef(target, c), nil)))), c);
}

/*
The `lookup` op has two arguments, a symbol and a register. It looks up the
value of the symbol in the environment pointed to by REnv and puts it in the
register.
*/
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  return
    EndWithLinkage(linkage,
      MakeSeq(REnv, target,
        MakePair(mem, SymbolFor("lookup"),
        MakePair(mem, exp,
        MakePair(mem, RegRef(target, c), nil)))), c);
}

static Seq CompileDefinitions(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Seq defs = EmptySeq();
  while (!IsNil(exp)) {
    Val var = Head(mem, exp);
    exp = Tail(mem, exp);
    Val val = Head(mem, exp);
    exp = Tail(mem, exp);
    Seq def = CompileDefinition(var, val, target, linkage, c);
    defs = AppendSeq(defs, def, c);
  }
  return defs;
}

/*
The `define` op has one argument, a symbol. It defines the symbol as the value
in RVal, in the environment pointed to by in REnv. It puts :ok in RVal.
*/
static Seq CompileDefinition(Val var, Val val, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  return
    EndWithLinkage(linkage,
      Preserving(REnv,
        CompileExp(val, RVal, SymbolFor("next"), c),
        MakeSeq(REnv | RVal, target,
          MakePair(mem, SymbolFor("define"),
          MakePair(mem, var, nil))), c), c);
}

/*
The `branch` op has one argument, a label. When the value in RVal is
false, it branches to the label.
*/
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val predicate = ListAt(mem, exp, 0);
  Val consequent = ListAt(mem, exp, 1);
  Val alternative = ListAt(mem, exp, 2);

  Val true_label = MakeLabel(c);
  Val false_label = MakeLabel(c);
  Val after_label = MakeLabel(c);

  Linkage consequent_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;
  Seq predicate_code = CompileExp(predicate, RVal, LinkNext, c);
  Seq consequent_code = CompileExp(consequent, target, consequent_linkage, c);
  Seq alternative_code = CompileExp(alternative, target, linkage, c);

  Seq test_seq = MakeSeq(RVal, 0,
    MakePair(mem, SymbolFor("branch"),
    MakePair(mem, LabelRef(false_label, c), nil)));
  Seq branches = ParallelSeq(
    AppendSeq(LabelSeq(true_label, c), consequent_code, c),
    AppendSeq(LabelSeq(false_label, c), alternative_code, c), c);

  return Preserving(REnv | RCon,
                    predicate_code,
                    AppendSeq(test_seq,
                    AppendSeq(branches,
                              LabelSeq(after_label, c), c), c), c);
}

static Seq CompileAnd(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val a = ListAt(mem, exp, 0);
  Val b = ListAt(mem, exp, 1);
  Val after_label = MakeLabel(c);

  Seq a_code = CompileExp(a, RVal, LinkNext, c);
  Seq b_code = CompileExp(b, RVal, LinkNext, c);
  Seq test_seq = MakeSeq(RVal, 0,
    MakePair(mem, SymbolFor("branch"),
    MakePair(mem, LabelRef(after_label, c), nil)));

  return
    AppendSeq(a_code,
    AppendSeq(test_seq,
    AppendSeq(b_code,
      LabelSeq(after_label, c), c), c), c);
}

/*
The `not` op tests the value in RVal. If the value is true, it writes :false
to RVal, otherwise it writes :true
*/
static Seq CompileOr(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val a = ListAt(mem, exp, 0);
  Val b = ListAt(mem, exp, 1);
  Val after_label = MakeLabel(c);

  Seq a_code = CompileExp(a, RVal, LinkNext, c);
  Seq b_code = CompileExp(b, RVal, LinkNext, c);

  Seq test_seq = MakeSeq(RVal, RVal,
    MakePair(mem, SymbolFor("not"),
    MakePair(mem, SymbolFor("branch"),
    MakePair(mem, LabelRef(after_label, c), nil))));

  return
    AppendSeq(a_code,
    AppendSeq(test_seq,
    AppendSeq(b_code,
      LabelSeq(after_label, c), c), c), c);
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

/*
The `lambda` op has two parameters, a label and a register name. It creates a
procedure object with a body at the label and the environment in REnv. A
pointer to the procedure is put in the target.
*/
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val proc_label = MakeLabel(c);
  Val after_label = MakeLabel(c);
  Linkage lambda_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;

  return
    AppendSeq(
      TackOn(
        EndWithLinkage(lambda_linkage,
          MakeSeq(REnv, target,
            MakePair(mem, SymbolFor("lambda"),
            MakePair(mem, LabelRef(proc_label, c),
            MakePair(mem, RegRef(target, c), nil)))), c),
        CompileLambdaBody(exp, proc_label, c), c),
      LabelSeq(after_label, c), c);
}

/*
The `extenv` pushes a new empty frame as the head of the list in REnv. The
result is saved in REnv.
The `defarg` has a symbol argument. It pops a value from the list in RArg
and defines the symbol to that value in the environment in REnv.
*/
static Seq CompileLambdaBody(Val exp, Val label, Compiler *c)
{
  Mem *mem = c->mem;
  Val params = ListAt(mem, exp, 0);
  Val body = ListAt(mem, exp, 1);

  Val bindings = nil;
  while (!IsNil(params)) {
    Val param = Head(mem, params);
    if (IsTerm(param, mem)) param = TermVal(param, mem);
    bindings =
      MakePair(mem, SymbolFor("defarg"),
      MakePair(mem, param, bindings));
    params = Tail(mem, params);
  }

  return
    AppendSeq(
      MakeSeq(REnv | RFun | RArg, REnv,
        MakePair(mem, MakePair(mem, SymbolFor("label"), label),
        MakePair(mem, SymbolFor("extenv"), bindings))),
      CompileExp(body, RVal, LinkReturn, c), c);
}

static Seq CompileImport(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  return CompileError("Imports not yet supported", nil, c);
}

/*
The `pusharg` op takes the value in RVal and makes it the head of the list
pointed to by RArg, storing the new list in RArg.
*/
static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Seq proc_code = CompileExp(Head(mem, exp), RFun, LinkNext, c);
  exp = ReverseOnto(mem, Tail(mem, exp), nil);

  Seq args;
  if (IsNil(exp)) {
    args =
      MakeSeq(0, RArg,
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, nil,
        MakePair(mem, RegRef(RArg, c), nil))));
  } else {
    Seq last_arg =
      AppendSeq(
        CompileExp(Head(mem, exp), RVal, LinkNext, c),
        MakeSeq(RVal, RArg,
          MakePair(mem, SymbolFor("const"),
          MakePair(mem, nil,
          MakePair(mem, RegRef(RArg, c),
          MakePair(mem, SymbolFor("pusharg"), nil))))), c);

    exp = Tail(mem, exp);
    if (IsNil(exp)) {
      args = last_arg;
    } else {
      Seq rest_args = CompileArgs(exp, c);
      args = Preserving(REnv, last_arg, rest_args, c);
    }
  }

  return
    Preserving(REnv | RCon,
      proc_code,
      Preserving(RFun | RCon,
        args,
        CompileCall(target, linkage, c), c), c);
}

static Seq CompileArgs(Val exp, Compiler *c)
{
  Mem *mem = c->mem;
  Seq next_arg =
    Preserving(RArg,
      CompileExp(Head(mem, exp), RVal, LinkNext, c),
      MakeSeq(RVal | RArg, RArg,
        MakePair(mem, SymbolFor("pusharg"), nil)), c);
  if (IsNil(Tail(mem, exp))) {
    return next_arg;
  } else {
    return
      Preserving(REnv,
        next_arg,
        CompileArgs(Tail(mem, exp), c), c);
  }
}

/*
The `brprim` op checks if the value in RFun is a primitive, and
branches to the given label if so.
The `prim` op has a register argument. It applies the primitive procedure in RFun to the
arguments in RArg, putting the result in the target register.
*/
static Seq CompileCall(Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  Val primitive_label = MakeLabel(c);
  Val compiled_label = MakeLabel(c);
  Val after_label = MakeLabel(c);

  Linkage compiled_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;

  return
    AppendSeq(
    AppendSeq(
      MakeSeq(RFun, 0,
        MakePair(mem, SymbolFor("brprim"),
        MakePair(mem, LabelRef(primitive_label, c), nil))),
      ParallelSeq(
        AppendSeq(
          LabelSeq(compiled_label, c),
          CompileProcAppl(target, compiled_linkage, c), c),
        AppendSeq(
          LabelSeq(primitive_label, c),
          EndWithLinkage(linkage,
            MakeSeq(RFun | RArg, target,
              MakePair(mem, SymbolFor("prim"),
              MakePair(mem, RegRef(target, c), nil))), c), c), c), c),
    LabelSeq(after_label, c), c);
}

/*
The `apply` op jumps to the label for the procedure in RFun
The `move` op has two register name parameters. It moves the value in the first
register to the second register.
*/
static Seq CompileProcAppl(Reg target, Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  if (target != RVal) {
    Assert(!Eq(linkage, SymbolFor("return")));
    Val proc_return = MakeLabel(c);
    return
      MakeSeq(RFun, RAll,
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, LabelRef(proc_return, c),
        MakePair(mem, RegRef(RCon, c),
        MakePair(mem, SymbolFor("apply"),
        MakePair(mem, MakePair(mem, SymbolFor("label"), proc_return),
        MakePair(mem, SymbolFor("move"),
        MakePair(mem, RegRef(target, c),
        MakePair(mem, RegRef(RVal, c),
        MakePair(mem, SymbolFor("jump"),
        MakePair(mem, LabelRef(linkage, c), nil)))))))))));
  }

  // tail recursive call
  if (Eq(linkage, SymbolFor("return"))) {
    return MakeSeq(RFun | RCon, RAll, MakePair(mem, SymbolFor("apply"), nil));
  }

  return
    MakeSeq(RFun, RAll,
      MakePair(mem, SymbolFor("const"),
      MakePair(mem, LabelRef(linkage, c),
      MakePair(mem, RegRef(RCon, c),
      MakePair(mem, SymbolFor("apply"), nil)))));
}

/*
The `jump` op has one arg, a label. It jumps to that label.
The `return` op jumps to the label in RCon
*/
static Seq CompileLinkage(Linkage linkage, Compiler *c)
{
  Mem *mem = c->mem;
  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RCon, 0,
      MakePair(mem, SymbolFor("return"), nil));
  }

  if (Eq(linkage, LinkNext)) {
    return EmptySeq(mem);
  }

  return MakeSeq(0, 0,
    MakePair(mem, SymbolFor("jump"),
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

/*
The `push` and `pop` ops have one register argument. `push` pushes the value in
the register on the stack, and `pop` pops the stack into the register.
*/
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
            MakePair(mem, SymbolFor("push"),
            MakePair(mem, RegRef(reg, c), seq1.stmts)),
            MakePair(mem, SymbolFor("pop"),
            MakePair(mem, RegRef(reg, c), nil)))),
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

static Val LabelRef(Val label, Compiler *c)
{
  Mem *mem = c->mem;
  return MakePair(mem, SymbolFor("label-ref"), label);
}

static Seq LabelSeq(Val label, Compiler *c)
{
  Mem *mem = c->mem;
  return MakeSeq(0, 0,
    MakePair(mem, MakePair(mem, SymbolFor("label"), label), nil));
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

void PrintSeq(Val stmts, Mem *mem)
{
  i32 i = 0;
  while (!IsNil(stmts)) {
    Val stmt = Head(mem, stmts);

    if (IsTagged(mem, stmt, "label")) {
      Print(SymbolName(mem, Tail(mem, stmt)));
      Print("\n");
    } else {
      PrintIntN(i, 4, ' ');
      Print("│ ");

      if (IsSym(stmt)) {
        PrintLn(SymbolName(mem, stmt));
      } else if (IsTagged(mem, stmt, "label-ref")) {
        Print(":");
        PrintLn(SymbolName(mem, Tail(mem, stmt)));
      } else if (IsTagged(mem, stmt, "reg")) {
        i32 reg = RawInt(Tail(mem, stmt));
        PrintReg(reg);
        Print("\n");
      } else {
        PrintVal(mem, stmt);
        Print("\n");
      }
    }

    i++;
    stmts = Tail(mem, stmts);
  }
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
    if (IsTerm(val, mem)) {
      PrintTerm(val, mem);
    } else {
      PrintVal(mem, val);
    }
  }
  Print("\n");
  PrintEscape(IOFGReset);
  return (Seq){false, 0, 0, nil};
}
