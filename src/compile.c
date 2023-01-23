#include "compile.h"
#include "eval.h"
#include "mem.h"
#include "inst.h"
#include "printer.h"

InstSeq CompileLinkage(Val linkage)
{
  const Val link_return = SymbolFor("return");
  const Val link_next = SymbolFor("next");

  if (Eq(linkage, link_return)) {
    return MakeInstSeq(1, OP_BRANCH);
  } else if (Eq(linkage, link_next)) {
    return EmptySeq();
  } else {
    return MakeInstSeq(2,
      OP_ASSIGN, linkage, REG_CONT,
      OP_BRANCH);
  }
}

InstSeq EndWithLinkage(Val linkage, InstSeq seq)
{
  return Preserving(WithReg(0, REG_CONT), seq, CompileLinkage(linkage));
}

InstSeq CompileSelfEval(Val exp, Reg target, Val linkage)
{
  return EndWithLinkage(linkage, MakeInstSeq(1, OP_ASSIGN, exp, target));
}

InstSeq CompileQuoted(Val exp, Reg target, Val linkage)
{
  return EndWithLinkage(linkage, MakeInstSeq(1, OP_ASSIGN, Tail(exp), target));
}

InstSeq CompileVariable(Val exp, Reg target, Val linkage)
{
  return EndWithLinkage(linkage, MakeInstSeq(1, OP_LOOKUP, exp));
}

InstSeq CompileDefine(Val exp, Reg target, Val linkage)
{
  Val var = Second(exp);
  InstSeq val_code = Compile(Third(exp), REG_VAL, SymbolFor("next"));
  Disassemble("val_code", val_code);
  InstSeq def_code = MakeInstSeq(1, OP_DEFINE, var);
  Disassemble("def_code", def_code);
  InstSeq pres_code = Preserving(WithReg(0, REG_ENV), val_code, def_code);
  Disassemble("pres_code", pres_code);
  InstSeq seq = EndWithLinkage(linkage, pres_code);
  Disassemble("linked_code", seq);
  return seq;
}

InstSeq CompileIf(Val exp, Reg target, Val linkage);
InstSeq CompileLambda(Val exp, Reg target, Val linkage);
InstSeq CompileSequence(Val exp, Reg target, Val linkage);
InstSeq CompileApplication(Val exp, Reg target, Val linkage);

InstSeq Compile(Val exp, Reg target, Val linkage)
{
  InstSeq seq;

  printf("Compiling: %s\n", ValStr(exp));

  if (IsSelfEvaluating(exp)) {
    seq = CompileSelfEval(exp, target, linkage);
  } else if (IsSym(exp)) {
    seq = CompileVariable(exp, target, linkage);
  } else if (IsTagged(exp, "quote")) {
    seq = CompileQuoted(exp, target, linkage);
  } else if (IsTagged(exp, "def")) {
    seq = CompileDefine(exp, target, linkage);
  // } else if (IsTagged(exp, "if")) {
  //   seq = CompileIf(exp, target, linkage);
  // } else if (IsTagged(exp, "->")) {
  //   seq = CompileLambda(exp, target, linkage);
  // } else if (IsTagged(exp, "do")) {
  //   seq = CompileSequence(exp, target, linkage);
  // } else if (IsList(exp)) {
  //   seq = CompileApplication(exp, target, linkage);
  } else {
    Error("Unknown expression");
  }

  Disassemble("Compiled", seq);
  printf("\n");

  return seq;
}
