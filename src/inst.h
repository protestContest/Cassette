#pragma once

typedef enum OpCode {
  OP_NOOP,
  OP_BRANCH,  // branch ; sets the pc to the value in reg cont
  OP_ASSIGN,  // const <val> <reg> ; loads a value into a reg
  OP_SAVE,    // save <reg> ; pushes value in reg onto the stack
  OP_RESTORE, // restore <reg> ; pulls value from the stack into reg
  OP_LOOKUP,  // lookup <val> ; looks up val in env reg, stores in val reg
  OP_DEFINE,  // define <var> ; defines symbol var to contents of reg val, in environment env reg
} OpCode;

typedef struct {
  u32 needs;
  u32 modifies;
  u32 size;
  u8 *code;
} InstSeq;

#define IsRegNeeded(seq, reg)   ((seq).needs & (1 << (reg)))
#define IsRegModified(seq, reg) ((seq).modifies & (1 << (reg)))
#define WithReg(regs, reg)      ((regs) | (1 << (reg)))
#define WithoutReg(regs, reg)   ((regs) & ~(1 << (reg)))

u32 PutInst(InstSeq seq, u32 i, OpCode op, ...);
InstSeq MakeInstSeq(u32 num_ops, ...);
InstSeq EmptySeq(void);

InstSeq AppendSeqs(InstSeq s1, InstSeq s2);
InstSeq Preserving(u32 regs, InstSeq s1, InstSeq s2);
InstSeq TackOnSeq(InstSeq s1, InstSeq s2);
InstSeq ParallelSeqs(InstSeq s1, InstSeq s2);

void Disassemble(char *title, InstSeq seq);
