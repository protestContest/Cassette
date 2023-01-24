#pragma once
#include "value.h"

enum Reg;

typedef enum {
  OP_LABEL,   // label <val> ; saves a position as a label
  OP_MOVE,    // move <reg> <reg> ; copies a reg to a reg
  OP_ASSIGN,  // assign <val> <reg> ; loads a value into a reg
  OP_TEST,    // test <reg> ; sets the flag based on a reg
  OP_BRANCH,  // branch <val> ; sets the pc to the value if flag is set
  OP_GOTO,    // goto <val> ; sets the pc to the value
  OP_SAVE,    // save <reg> ; pushes value in reg onto the stack
  OP_RESTORE, // restore <reg> ; pulls value from the stack into reg
  OP_ADD,     // add <val> <reg> ; adds val to reg
} OpCode;

typedef enum {
  ARGS_NONE,
  ARGS_REG,
  ARGS_VAL,
  ARGS_REG_REG,
  ARGS_VAL_REG,
} ArgFormat;

typedef struct Label {
  Val name;
  u32 pos;
  struct Label *next;
} Label;

typedef struct {
  u32 needs;
  u32 modifies;
  u32 size;
  u32 pos;
  u8 *code;
} Chunk;

#define IsRegNeeded(seq, reg)   ((seq)->needs & (1 << (reg)))
#define IsRegModified(seq, reg) ((seq)->modifies & (1 << (reg)))
#define WithReg(regs, reg)      ((regs) | (1 << (reg)))
#define WithoutReg(regs, reg)   ((regs) & ~(1 << (reg)))

const char *OpStr(OpCode op);
ArgFormat OpFormat(OpCode op);
u32 OpSize(OpCode op);

void PutByte(Chunk *seq, u8 byte);
void PutVal(Chunk *seq, Val val);
u8 ReadByte(Chunk *seq);
Val ReadVal(Chunk *seq);
void PutInst(Chunk *seq, OpCode op, ...);

Chunk *MakeChunk(u32 num_ops, ...);
Chunk *EmptyChunk(void);

Chunk *AppendSeqs(Chunk *s1, Chunk *s2);
Chunk *Preserving(u32 regs, Chunk *s1, Chunk *s2);
Chunk *TackOnSeq(Chunk *s1, Chunk *s2);
Chunk *ParallelSeqs(Chunk *s1, Chunk *s2);

void Assemble(Chunk *chunk);
u32 DisassembleInstruction(Chunk *chunk);
void Disassemble(Chunk *chunk);
void PrintChunk(char *title, Chunk *chunk);
