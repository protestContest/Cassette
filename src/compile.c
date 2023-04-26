#include "compile.h"

#define RegVal    Bit(0)
#define RegExp    Bit(1)
#define RegEnv    Bit(2)
#define RegCont   Bit(3)
#define RegProc   Bit(4)
#define RegArgs   Bit(5)
#define RegTmp    Bit(6)
typedef u32 Reg;

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkNext    SymbolFor("next")

typedef struct {
  Reg needs;
  Reg modifies;
  Val stmts;
} Seq;

#define MakeSeq(needs, modifies, stmts)   (Seq){needs, modifies, stmts}
#define EmptySeq  MakeSeq(0, 0, nil)

#define Save(reg, mem)    MakePair(mem, MakeSymbol(mem, "push"), IntVal(reg))
#define Restore(reg, mem) MakePair(mem, MakeSymbol(mem, "pop"), IntVal(reg))

static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileQuoted(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileDefinition(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileCond(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileLambdaBody(Val exp, Seq label, Mem *mem);
static Seq CompileImport(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileArgs(Val exp, Mem *mem);
static Seq CompileCall(Reg target, Linkage linkage, Mem *mem);
static Seq CompileProcAppl(Reg target, Linkage linkage, Mem *mem);

static Seq CompileLinkage(Linkage linkage, Mem *mem);
static Seq EndWithLinkage(Linkage linkage, Seq seq, Mem *mem);
static Seq AppendSeq(Seq seq1, Seq seq2, Mem *mem);
static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Mem *mem);
static Seq TackOn(Seq seq1, Seq seq2, Mem *mem);
static Seq ParallelSeq(Seq seq1, Seq seq2, Mem *mem);

Seq CompileExp(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  if (IsNil(exp) || IsNumeric(exp) || IsObj(exp)) {
    return CompileSelf(exp, target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor(":"))) {
    return CompileQuoted(ListAt(mem, exp, 1), target, linkage, mem);
  } else if (IsSym(exp)) {
    return CompileVariable(exp, target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("let"))) {
    return CompileDefinition(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("if"))) {
    return CompileIf(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("cond"))) {
    return CompileCond(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("do"))) {
    return CompileSequence(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("->"))) {
    return CompileLambda(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("import"))) {
    return CompileImport(Tail(mem, exp), target, linkage, mem);
  } else {
    return CompileApplication(exp, target, linkage, mem);
  }
}

/*
The `assign` op puts a value in the target register
*/
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val stmts =
    MakePair(mem, MakeSymbol(mem, "assign"),
    MakePair(mem, exp,
    MakePair(mem, IntVal(target), nil)));
  return EndWithLinkage(linkage, MakeSeq(0, target, stmts), mem);
}

static Seq CompileQuoted(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val stmts =
    MakePair(mem, MakeSymbol(mem, "assign"),
    MakePair(mem, exp,
    MakePair(mem, IntVal(target), nil)));
  return EndWithLinkage(linkage, MakeSeq(0, target, stmts), mem);
}

/*
The `lookup` op has one argument, a symbol. It looks up the value of the symbol
in the environment pointed to by RegEnv and puts it in the target.
*/
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val stmts =
    MakePair(mem, MakeSymbol(mem, "lookup"),
    MakePair(mem, exp,
    MakePair(mem, IntVal(target), nil)));
  return EndWithLinkage(linkage, MakeSeq(RegEnv, target, stmts), mem);
}

/*
OpDefine has one argument, a symbol. It defines the symbol as the value in
RegVal, in the environment pointed to by in RegEnv. It puts :ok in RegVal.
*/
static Seq CompileDefinition(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val var = ListAt(mem, exp, 0);
  Val val = ListAt(mem, exp, 1);

  Seq val_code = CompileExp(val, RegVal, SymbolFor("next"), mem);
  Val stmts = MakePair(mem, MakeSymbol(mem, "define"), var);
  Seq def_code = MakeSeq(RegEnv | RegVal, target, stmts);
  return EndWithLinkage(linkage, Preserving(RegEnv, val_code, def_code, mem), mem);
}

static Seq MakeLabel(Mem *mem)
{
  static i32 label_num = 0;
  char label[8];
  u32 len = IntToString(label_num++, label, 8);
  label[len] = 0;
  return MakeSeq(0, 0, MakePair(mem, SymbolFor("label"), SymbolFor(label)));
}

/*
The `branch_false` op has one argument, a label. When the value in RegVal is
false, it branches to the label.
*/
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val predicate = ListAt(mem, exp, 0);
  Val consequent = ListAt(mem, exp, 1);
  Val alternative = ListAt(mem, exp, 2);

  Seq true_label = MakeLabel(mem);
  Seq false_label = MakeLabel(mem);
  Seq after_label = MakeLabel(mem);

  Linkage consequent_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label.stmts : linkage;
  Seq predicate_code = CompileExp(predicate, RegVal, LinkNext, mem);
  Seq consequent_code = CompileExp(consequent, target, consequent_linkage, mem);
  Seq alternative_code = CompileExp(alternative, target, linkage, mem);

  Seq test_seq = MakeSeq(RegVal, 0, MakePair(mem, SymbolFor("branch_false"), false_label.stmts));
  Seq branches = ParallelSeq(
    AppendSeq(true_label, consequent_code, mem),
    AppendSeq(false_label, alternative_code, mem), mem);

  return Preserving(RegEnv | RegCont,
                    predicate_code,
                    AppendSeq(test_seq,
                    AppendSeq(branches,
                              after_label, mem), mem), mem);
}

static Seq CompileCond(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return EmptySeq;
}

static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  if (IsNil(Tail(mem, exp))) {
    return CompileExp(Head(mem, exp), target, linkage, mem);
  }

  return Preserving(RegEnv | RegCont,
                    CompileExp(Head(mem, exp), target, LinkNext, mem),
                    CompileSequence(Tail(mem, exp), target, linkage, mem), mem);
}

/*
The `lambda` op has two parameters, a label and a register name. It creates a
procedure object with a body at the label and the environment in RegEnv. A
pointer to the procedure is put in the target.
*/
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Seq proc_label = MakeLabel(mem);
  Seq after_label = MakeLabel(mem);
  Linkage lambda_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label.stmts : linkage;

  return
    AppendSeq(
      TackOn(
        EndWithLinkage(lambda_linkage,
          MakeSeq(RegEnv, target,
            MakePair(mem, MakeSymbol(mem, "lambda"),
            MakePair(mem, proc_label.stmts,
            MakePair(mem, IntVal(target), nil)))), mem),
        CompileLambdaBody(exp, proc_label, mem), mem),
      after_label, mem);
}

/*
The `extend_env` has one parameter, "params". It assumes RegProc points to a
procedure. It fetches the environment saved with that procedure and extends it
by binding the params to the values in RegArgs. The new environment is stored in
RegEnv.
*/
static Seq CompileLambdaBody(Val exp, Seq label, Mem *mem)
{
  Val params = ListAt(mem, exp, 0);
  Val body = ListAt(mem, exp, 1);

  return
    AppendSeq(
      MakeSeq(RegEnv | RegProc | RegArgs, RegEnv,
        MakePair(mem, label.stmts,
        MakePair(mem, MakeSymbol(mem, "extend_env"),
        MakePair(mem, params, nil)))),
      CompileExp(body, RegVal, LinkReturn, mem), mem);
}

static Seq CompileImport(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return EmptySeq;
}

/*
The `push_arg` op takes the value in RegVal and makes it the head of the list
pointed to by RegArgs, storing the new list in RegArgs.
*/
static Seq CompileApplication(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Seq proc_code = CompileExp(Head(mem, exp), RegProc, LinkNext, mem);
  exp = ReverseOnto(mem, Tail(mem, exp), nil);

  Seq args;
  if (IsNil(exp)) {
    args =
      MakeSeq(0, RegArgs,
        MakePair(mem, SymbolFor("assign"),
        MakePair(mem, nil,
        MakePair(mem, IntVal(RegArgs), nil))));
  } else {
    Seq last_arg =
      AppendSeq(
        CompileExp(Head(mem, exp), RegVal, LinkNext, mem),
        MakeSeq(RegVal, RegArgs,
          MakePair(mem, MakeSymbol(mem, "assign"),
          MakePair(mem, nil,
          MakePair(mem, IntVal(RegArgs),
          MakePair(mem, MakeSymbol(mem, "push_arg"), nil))))), mem);

    exp = Tail(mem, exp);
    if (IsNil(exp)) {
      args = last_arg;
    } else {
      Seq rest_args = CompileArgs(exp, mem);
      args = Preserving(RegEnv, last_arg, rest_args, mem);
    }
  }

  return
    Preserving(RegEnv | RegCont,
      proc_code,
      Preserving(RegProc | RegCont,
        args,
        CompileCall(target, linkage, mem), mem), mem);
}

static Seq CompileArgs(Val exp, Mem *mem)
{
  Seq next_arg =
    Preserving(RegArgs,
      CompileExp(Head(mem, exp), RegVal, LinkNext, mem),
      MakeSeq(RegVal | RegArgs, RegArgs,
        MakePair(mem, MakeSymbol(mem, "push_arg"), nil)), mem);
  if (IsNil(Tail(mem, exp))) {
    return next_arg;
  } else {
    return
      Preserving(RegEnv,
        next_arg,
        CompileArgs(Tail(mem, exp), mem), mem);
  }
}

static Seq CompileCall(Reg target, Linkage linkage, Mem *mem)
{
  Seq primitive_label = MakeLabel(mem);
  Seq compiled_label = MakeLabel(mem);
  Seq after_label = MakeLabel(mem);

  Linkage compiled_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label.stmts : linkage;

  return
    AppendSeq(
    AppendSeq(
      MakeSeq(RegProc, 0,
        MakePair(mem, MakeSymbol(mem, "branch_primitive"),
        MakePair(mem, primitive_label.stmts, nil))),
      ParallelSeq(
        AppendSeq(
          compiled_label,
          CompileProcAppl(target, compiled_linkage, mem), mem),
        AppendSeq(
          primitive_label,
          EndWithLinkage(linkage,
            MakeSeq(RegProc | RegArgs, target,
              MakePair(mem, MakeSymbol(mem, "apply_primitive"), nil)), mem), mem), mem), mem),
    after_label, mem);
}

static Seq CompileProcAppl(Reg target, Linkage linkage, Mem *mem)
{

}

static Seq CompileLinkage(Linkage linkage, Mem *mem)
{
  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RegCont, 0, LinkReturn);
  }

  if (Eq(linkage, LinkNext)) {
    return EmptySeq;
  }

  return MakeSeq(0, 0, MakePair(mem, MakeSymbol(mem, "jump"), linkage));
}

static Seq EndWithLinkage(Linkage linkage, Seq seq, Mem *mem)
{
  return Preserving(RegCont, seq, CompileLinkage(linkage, mem), mem);
}

static Seq AppendSeq(Seq seq1, Seq seq2, Mem *mem)
{
  Reg needs = seq1.needs | (seq2.needs & ~seq1.modifies);
  Reg modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Mem *mem)
{
  if (regs == 0) {
    return AppendSeq(seq1, seq2, mem);
  }

  Reg reg = 0;
  for (u32 i = 0; i < 7; i++) {
    if (regs & Bit(i)) {
      reg = i;
      break;
    }
  }

  regs = regs & ~reg;
  if ((seq2.needs & reg) && (seq1.modifies & reg)) {
    Reg needs = reg | seq1.needs;
    Reg modifies = seq1.modifies & ~reg;
    Val stmts =
      MakePair(mem, Save(reg, mem),
      MakePair(mem, seq1.stmts,
      MakePair(mem, Restore(reg, mem), nil)));
    Seq seq = MakeSeq(needs, modifies, stmts);

    return Preserving(regs, seq, seq2, mem);
  } else {
    return Preserving(regs, seq1, seq2, mem);
  }
}

static Seq TackOn(Seq seq1, Seq seq2, Mem *mem)
{
  return MakeSeq(seq1.needs, seq1.modifies, ListAppend(mem, seq1.stmts, seq2.stmts));
}

static Seq ParallelSeq(Seq seq1, Seq seq2, Mem *mem)
{
  Reg needs = seq1.needs | seq2.needs;
  Reg modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}
