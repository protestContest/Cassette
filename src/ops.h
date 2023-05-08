#pragma once
#include "mem.h"

/*
Op codes

halt                halts execution
const [n] [reg]     loads constant #n into register reg
test [reg]          sets condition flag when value in [reg] is not false or nil
not [reg]           sets condition flag when value in [reg] is false or nil
branch [n]          jumps to position [n] when condition flag is set
jump [n]            jumps to position [n]
goto [reg]          jumps to position in [reg] (error if not an int)
push [reg1] [reg2]  extends list in [reg2] with value in [reg1] as the head
pop [reg1] [reg2]   puts the head of list in [reg2] into [reg1]; replaces [reg2] with its tail
*/

typedef enum {
  OpNoop,
  OpHalt,
  OpConst,
  OpTest,
  OpNot,
  OpBranch,
  OpJump,
  OpGoto,
  OpPush,
  OpPop,
  OpHead,
  OpTail,
  OpPair,

  NUM_OPCODES
} OpCode;

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstReg,
  ArgsRegReg,
} ArgInfo;

char *OpName(OpCode op);
ArgInfo OpArgs(OpCode op);
u32 OpLength(OpCode op);
void InitOps(Mem *mem);
u32 PrintOpCode(OpCode op);
