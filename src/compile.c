#include "compile.h"

typedef Val Linkage;
#define LinkReturn  SymbolFor("return")
#define LinkNext    SymbolFor("next")

typedef struct {
  Reg needs;
  Reg modifies;
  Val stmts;
} Seq;

#define MakeSeq(needs,  modifies, stmts)   (Seq){needs, modifies, stmts}
#define EmptySeq(mem)   MakeSeq(0, 0, nil)
#define IsEmptySeq(seq) (IsNil((seq).stmts))

static Seq CompileExp(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileDefinition(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileCond(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileSequence(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileLambda(Val exp, Reg target, Linkage linkage, Mem *mem);
static Seq CompileLambdaBody(Val exp, Val label, Mem *mem);
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
static Val MakeLabel(Mem *mem);
static Val LabelRef(Val label, Mem *mem);
static Seq LabelSeq(Val label, Mem *mem);
static Val RegRef(Reg reg, Mem *mem);
Val ExtractLabels(Val stmts, Mem *mem);
void CompileError(char *message, Val val, Mem *mem);

Val Compile(Val exp, Mem *mem)
{
  // symbols for opcodes
  MakeSymbol(mem, "noop");
  MakeSymbol(mem, "const");
  MakeSymbol(mem, "lookup");
  MakeSymbol(mem, "define");
  MakeSymbol(mem, "branch");
  MakeSymbol(mem, "lambda");
  MakeSymbol(mem, "defarg");
  MakeSymbol(mem, "extenv");
  MakeSymbol(mem, "pusharg");
  MakeSymbol(mem, "brprim");
  MakeSymbol(mem, "prim");
  MakeSymbol(mem, "apply");
  MakeSymbol(mem, "move");
  MakeSymbol(mem, "push");
  MakeSymbol(mem, "pop");
  MakeSymbol(mem, "jump");
  MakeSymbol(mem, "return");
  MakeSymbol(mem, "halt");

  // next linkage
  MakeSymbol(mem, "next");

  // AST tags
  MakeSymbol(mem, ":");
  MakeSymbol(mem, "let");
  MakeSymbol(mem, "if");
  MakeSymbol(mem, "cond");
  MakeSymbol(mem, "do");
  MakeSymbol(mem, "->");
  MakeSymbol(mem, "import");

  // utility tags
  MakeSymbol(mem, "label");
  MakeSymbol(mem, "label-ref");
  MakeSymbol(mem, "reg");

  Val done = MakeLabel(mem);
  Seq seq =
    AppendSeq(CompileExp(exp, RegVal, done, mem),
    AppendSeq(LabelSeq(done, mem),
              MakeSeq(0, 0, MakePair(mem, SymbolFor("halt"), nil)), mem), mem);

  return ExtractLabels(seq.stmts, mem);
}

#ifdef DEBUG_COMPILE
static u32 indent = 0;
#endif
static Seq CompileExp(Val exp, Reg target, Linkage linkage, Mem *mem)
{
#ifdef DEBUG_COMPILE
  Print("\n");
  for (u32 i = 0; i < indent; i++) Print("  ");
  Print("Compile: ");
  PrintVal(mem, exp);
  indent++;
#endif

  Assert(!IsObj(exp));

  Seq result;
  if (IsNil(exp) || IsNumeric(exp) || IsObj(exp)) {
#ifdef DEBUG_COMPILE
    Print(" (Self)");
#endif
    result = CompileSelf(exp, target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor(":"))) {
#ifdef DEBUG_COMPILE
    Print(" (Quote)");
#endif
    result = CompileSelf(ListAt(mem, exp, 1), target, linkage, mem);
  } else if (IsSym(exp)) {
#ifdef DEBUG_COMPILE
    Print(" (Variable)");
#endif
    result = CompileVariable(exp, target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("let"))) {
#ifdef DEBUG_COMPILE
    Print(" (Definition)");
#endif
    result = CompileDefinition(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("if"))) {
#ifdef DEBUG_COMPILE
    Print(" (If)");
#endif
    result = CompileIf(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("cond"))) {
#ifdef DEBUG_COMPILE
    Print(" (Cond)");
#endif
    result = CompileCond(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("do"))) {
#ifdef DEBUG_COMPILE
    Print(" (Sequence)");
#endif
    result = CompileSequence(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("->"))) {
#ifdef DEBUG_COMPILE
    Print(" (Lambda)");
#endif
    result = CompileLambda(Tail(mem, exp), target, linkage, mem);
  } else if (IsTagged(mem, exp, SymbolFor("import"))) {
#ifdef DEBUG_COMPILE
    Print(" (Import)");
#endif
    result = CompileImport(Tail(mem, exp), target, linkage, mem);
  } else {
#ifdef DEBUG_COMPILE
    Print(" (Application)");
#endif
    result = CompileApplication(exp, target, linkage, mem);
  }

#ifdef DEBUG_COMPILE
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
static Seq CompileSelf(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return
    EndWithLinkage(linkage,
      MakeSeq(0, target,
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, exp,
        MakePair(mem, RegRef(target, mem), nil)))), mem);
}

/*
The `lookup` op has two arguments, a symbol and a register. It looks up the
value of the symbol in the environment pointed to by RegEnv and puts it in the
register.
*/
static Seq CompileVariable(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return
    EndWithLinkage(linkage,
      MakeSeq(RegEnv, target,
        MakePair(mem, SymbolFor("lookup"),
        MakePair(mem, exp,
        MakePair(mem, RegRef(target, mem), nil)))), mem);
}

/*
The `define` op has one argument, a symbol. It defines the symbol as the value
in RegVal, in the environment pointed to by in RegEnv. It puts :ok in RegVal.
*/
static Seq CompileDefinition(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val var = ListAt(mem, exp, 0);
  Val val = ListAt(mem, exp, 1);

  return
    EndWithLinkage(linkage,
      Preserving(RegEnv,
        CompileExp(val, RegVal, SymbolFor("next"), mem),
        MakeSeq(RegEnv | RegVal, target,
          MakePair(mem, SymbolFor("define"),
          MakePair(mem, var, nil))), mem), mem);
}

/*
The `branch` op has one argument, a label. When the value in RegVal is
false, it branches to the label.
*/
static Seq CompileIf(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  Val predicate = ListAt(mem, exp, 0);
  Val consequent = ListAt(mem, exp, 1);
  Val alternative = ListAt(mem, exp, 2);

  Val true_label = MakeLabel(mem);
  Val false_label = MakeLabel(mem);
  Val after_label = MakeLabel(mem);

  Linkage consequent_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;
  Seq predicate_code = CompileExp(predicate, RegVal, LinkNext, mem);
  Seq consequent_code = CompileExp(consequent, target, consequent_linkage, mem);
  Seq alternative_code = CompileExp(alternative, target, linkage, mem);

  Seq test_seq = MakeSeq(RegVal, 0,
    MakePair(mem, SymbolFor("branch"),
    MakePair(mem, LabelRef(false_label, mem), nil)));
  Seq branches = ParallelSeq(
    AppendSeq(LabelSeq(true_label, mem), consequent_code, mem),
    AppendSeq(LabelSeq(false_label, mem), alternative_code, mem), mem);

  return Preserving(RegEnv | RegCont,
                    predicate_code,
                    AppendSeq(test_seq,
                    AppendSeq(branches,
                              LabelSeq(after_label, mem), mem), mem), mem);
}

static Seq CompileCond(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return EmptySeq(mem);
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
  Val proc_label = MakeLabel(mem);
  Val after_label = MakeLabel(mem);
  Linkage lambda_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;

  return
    AppendSeq(
      TackOn(
        EndWithLinkage(lambda_linkage,
          MakeSeq(RegEnv, target,
            MakePair(mem, SymbolFor("lambda"),
            MakePair(mem, LabelRef(proc_label, mem),
            MakePair(mem, RegRef(target, mem), nil)))), mem),
        CompileLambdaBody(exp, proc_label, mem), mem),
      LabelSeq(after_label, mem), mem);
}

/*
The `extenv` pushes a new empty frame as the head of the list in RegEnv. The
result is saved in RegEnv.
The `defarg` has a symbol argument. It pops a value from the list in RegArgs
and defines the symbol to that value in the environment in RegEnv.
*/
static Seq CompileLambdaBody(Val exp, Val label, Mem *mem)
{
  Val params = ListAt(mem, exp, 0);
  Val body = ListAt(mem, exp, 1);

  Val bindings = nil;
  while (!IsNil(params)) {
    Val param = Head(mem, params);
    bindings =
      MakePair(mem, SymbolFor("defarg"),
      MakePair(mem, param, bindings));
    params = Tail(mem, params);
  }

  return
    AppendSeq(
      MakeSeq(RegEnv | RegProc | RegArgs, RegEnv,
        MakePair(mem, MakePair(mem, SymbolFor("label"), label),
        MakePair(mem, SymbolFor("extenv"), bindings))),
      CompileExp(body, RegVal, LinkReturn, mem), mem);
}

static Seq CompileImport(Val exp, Reg target, Linkage linkage, Mem *mem)
{
  return EmptySeq(mem);
}

/*
The `pusharg` op takes the value in RegVal and makes it the head of the list
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
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, nil,
        MakePair(mem, RegRef(RegArgs, mem), nil))));
  } else {
    Seq last_arg =
      AppendSeq(
        CompileExp(Head(mem, exp), RegVal, LinkNext, mem),
        MakeSeq(RegVal, RegArgs,
          MakePair(mem, SymbolFor("const"),
          MakePair(mem, nil,
          MakePair(mem, RegRef(RegArgs, mem),
          MakePair(mem, SymbolFor("pusharg"), nil))))), mem);

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
        MakePair(mem, SymbolFor("pusharg"), nil)), mem);
  if (IsNil(Tail(mem, exp))) {
    return next_arg;
  } else {
    return
      Preserving(RegEnv,
        next_arg,
        CompileArgs(Tail(mem, exp), mem), mem);
  }
}

/*
The `brprim` op checks if the value in RegProc is a primitive, and
branches to the given label if so.
The `prim` op applies the primitive procedure in RegProc to the
arguments in RegArgs
*/
static Seq CompileCall(Reg target, Linkage linkage, Mem *mem)
{
  Val primitive_label = MakeLabel(mem);
  Val compiled_label = MakeLabel(mem);
  Val after_label = MakeLabel(mem);

  Linkage compiled_linkage = (Eq(linkage, SymbolFor("next"))) ? after_label : linkage;

  return
    AppendSeq(
    AppendSeq(
      MakeSeq(RegProc, 0,
        MakePair(mem, SymbolFor("brprim"),
        MakePair(mem, LabelRef(primitive_label, mem), nil))),
      ParallelSeq(
        AppendSeq(
          LabelSeq(compiled_label, mem),
          CompileProcAppl(target, compiled_linkage, mem), mem),
        AppendSeq(
          LabelSeq(primitive_label, mem),
          EndWithLinkage(linkage,
            MakeSeq(RegProc | RegArgs, target,
              MakePair(mem, SymbolFor("prim"), nil)), mem), mem), mem), mem),
    LabelSeq(after_label, mem), mem);
}

/*
The `apply` op sets RegVal to the procedure body label for the procedure in
RegProc, then jumps to RegVal.
The `move` op has two register name parameters. It moves the value in the first
register to the second register.
*/
static Seq CompileProcAppl(Reg target, Linkage linkage, Mem *mem)
{
  if (target != RegVal) {
    Assert(!Eq(linkage, SymbolFor("return")));
    Val proc_return = MakeLabel(mem);
    return
      MakeSeq(RegProc, RegAll,
        MakePair(mem, SymbolFor("const"),
        MakePair(mem, LabelRef(proc_return, mem),
        MakePair(mem, RegRef(RegCont, mem),
        MakePair(mem, SymbolFor("apply"),
        MakePair(mem, MakePair(mem, SymbolFor("label"), proc_return),
        MakePair(mem, SymbolFor("move"),
        MakePair(mem, RegRef(target, mem),
        MakePair(mem, RegRef(RegVal, mem),
        MakePair(mem, SymbolFor("jump"),
        MakePair(mem, LabelRef(linkage, mem), nil)))))))))));
  }

  // tail recursive call
  if (Eq(linkage, SymbolFor("return"))) {
    return MakeSeq(RegProc | RegCont, RegAll, MakePair(mem, SymbolFor("apply"), nil));
  }

  return
    MakeSeq(RegProc, RegAll,
      MakePair(mem, SymbolFor("const"),
      MakePair(mem, LabelRef(linkage, mem),
      MakePair(mem, RegRef(RegCont, mem),
      MakePair(mem, SymbolFor("apply"), nil)))));
}

/*
The `jump` op has one arg, a label. It jumps to that label.
The `return` op jumps to the label in RegCont
*/
static Seq CompileLinkage(Linkage linkage, Mem *mem)
{
  if (Eq(linkage, LinkReturn)) {
    return MakeSeq(RegCont, 0,
      MakePair(mem, SymbolFor("return"), nil));
  }

  if (Eq(linkage, LinkNext)) {
    return EmptySeq(mem);
  }

  return MakeSeq(0, 0,
    MakePair(mem, SymbolFor("jump"),
    MakePair(mem, LabelRef(linkage, mem), nil)));
}

static Seq EndWithLinkage(Linkage linkage, Seq seq, Mem *mem)
{
  return Preserving(RegCont, seq, CompileLinkage(linkage, mem), mem);
}

static Seq AppendSeq(Seq seq1, Seq seq2, Mem *mem)
{
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
static Seq Preserving(Reg regs, Seq seq1, Seq seq2, Mem *mem)
{
  if (regs == 0) {
    return AppendSeq(seq1, seq2, mem);
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
            MakePair(mem, RegRef(reg, mem), seq1.stmts)),
            MakePair(mem, SymbolFor("pop"),
            MakePair(mem, RegRef(reg, mem), nil)))),
        seq2, mem);
  } else {
    return Preserving(regs, seq1, seq2, mem);
  }
}

static Seq TackOn(Seq seq1, Seq seq2, Mem *mem)
{
  return MakeSeq(seq1.needs, seq1.modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static Seq ParallelSeq(Seq seq1, Seq seq2, Mem *mem)
{
  Reg needs = seq1.needs | seq2.needs;
  Reg modifies = seq1.modifies | seq2.modifies;
  return MakeSeq(needs, modifies, ListConcat(mem, seq1.stmts, seq2.stmts));
}

static i32 label_num = 0;
static Val MakeLabel(Mem *mem)
{
  char label[12] = "lbl";
  u32 len = IntToString(label_num++, label+3, 7);
  label[len+3] = 0;
  return MakeSymbol(mem, label);
}

static Val LabelRef(Val label, Mem *mem)
{
  return MakePair(mem, SymbolFor("label-ref"), label);
}

static Seq LabelSeq(Val label, Mem *mem)
{
  return MakeSeq(0, 0,
    MakePair(mem, MakePair(mem, SymbolFor("label"), label), nil));
}

static Val RegRef(Reg reg, Mem *mem)
{
  return MakePair(mem, SymbolFor("reg"), IntVal(reg));
}

void PrintSeq(Val stmts, Mem *mem)
{
  i32 i = 0;
  while (!IsNil(stmts)) {
    PrintIntN(i, 4, ' ');
    Print("│ ");

    Val stmt = Head(mem, stmts);

    if (IsSym(stmt)) {
      Print("  ");
      PrintLn(SymbolName(mem, stmt));
    } else if (IsTagged(mem, stmt, SymbolFor("label"))) {
      Print(SymbolName(mem, Tail(mem, stmt)));
      PrintLn(":");
    } else if (IsTagged(mem, stmt, SymbolFor("label-ref"))) {
      Print("  ");
      Print(":");
      PrintLn(SymbolName(mem, Tail(mem, stmt)));
    } else if (IsTagged(mem, stmt, SymbolFor("reg"))) {
      Print("  ");
      i32 reg = RawInt(Tail(mem, stmt));
      PrintReg(reg);
      Print("\n");
    } else {
      Print("  ");
      PrintVal(mem, stmt);
      Print("\n");
    }

    i++;
    stmts = Tail(mem, stmts);
  }
}

Val ReplaceLabels(Val stmts, Map labels, Mem *mem)
{
  if (IsNil(stmts)) return nil;

  Val stmt = Head(mem, stmts);
  if (IsTagged(mem, stmt, SymbolFor("label"))) {
    return ReplaceLabels(Tail(mem, stmts), labels, mem);
  }

  if (IsTagged(mem, stmt, SymbolFor("label-ref"))) {
    Val label = Tail(mem, stmt);
    if (!MapContains(&labels, label.as_v)) {
      CompileError("Undefined label", label, mem);
    }
    u32 pos = MapGet(&labels, label.as_v);
    return MakePair(mem, IntVal(pos), ReplaceLabels(Tail(mem, stmts), labels, mem));
  }

  return MakePair(mem, stmt, ReplaceLabels(Tail(mem, stmts), labels, mem));
}

Val ExtractLabels(Val stmts, Mem *mem)
{
  Map labels;
  InitMap(&labels);

  u32 i = 0;
  Val cur = stmts;
  while (!IsNil(cur)) {
    Val stmt = Head(mem, cur);
    if (IsTagged(mem, stmt, SymbolFor("label"))) {
      Val label = Tail(mem, stmt);
      MapSet(&labels, label.as_v, i);
    } else {
      i++;
    }
    cur = Tail(mem, cur);
  }

  return ReplaceLabels(stmts, labels, mem);
}

void CompileError(char *message, Val val, Mem *mem)
{
  Print("(Compile Error) ");
  Print(message);
  Print(": ");
  PrintVal(mem, val);
  Print("\n");
}
