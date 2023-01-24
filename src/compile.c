#include "compile.h"
#include "eval.h"
#include "mem.h"
#include "chunk.h"
#include "printer.h"

Chunk *CompileLinkage(Val linkage)
{
  const Val link_return = SymbolFor("return");
  const Val link_next = SymbolFor("next");

  if (Eq(linkage, link_return)) {
    return MakeChunk(1, OP_BRANCH);
  } else if (Eq(linkage, link_next)) {
    return EmptyChunk();
  } else {
    return MakeChunk(2,
      OP_ASSIGN, linkage, REG_CONT,
      OP_BRANCH);
  }
}

Chunk *EndWithLinkage(Val linkage, Chunk *seq);
Chunk *CompileSelfEval(Val exp, Reg target, Val linkage);
Chunk *CompileQuoted(Val exp, Reg target, Val linkage);
Chunk CompileVariable(Val exp, Reg target, Val linkage);
Chunk CompileDefine(Val exp, Reg target, Val linkage);
Chunk CompileIf(Val exp, Reg target, Val linkage);
Chunk CompileLambda(Val exp, Reg target, Val linkage);
Chunk CompileSequence(Val exp, Reg target, Val linkage);
Chunk CompileApplication(Val exp, Reg target, Val linkage);

Chunk *Compile(Val exp, Reg target, Val linkage)
{
  Chunk *seq = EmptyChunk();

  printf("Compiling: %s\n", ValStr(exp));

  // if (IsSelfEvaluating(exp)) {
  //   seq = CompileSelfEval(exp, target, linkage);
  // // } else if (IsSym(exp)) {
  // //   seq = CompileVariable(exp, target, linkage);
  // } else if (IsTagged(exp, "quote")) {
  //   seq = CompileQuoted(exp, target, linkage);
  // } else if (IsTagged(exp, "def")) {
  //   seq = CompileDefine(exp, target, linkage);
  // } else if (IsTagged(exp, "if")) {
  //   seq = CompileIf(exp, target, linkage);
  // } else if (IsTagged(exp, "->")) {
  //   seq = CompileLambda(exp, target, linkage);
  // } else if (IsTagged(exp, "do")) {
  //   seq = CompileSequence(exp, target, linkage);
  // } else if (IsList(exp)) {
  //   seq = CompileApplication(exp, target, linkage);
  // } else {
  //   Error("Unknown expression");
  // }

  // Disassemble("Compiled", seq);
  // printf("\n");

  return seq;
}
