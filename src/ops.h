#pragma once
#include "mem.h"

/*
Op codes

halt                halts execution
const [n] [reg]     loads constant #n into register reg
move [reg1] [reg2]  copies value from reg1 to reg2
branch [n] [reg]    jumps to position n when reg is not false or nil
jump [n]            jumps to position n
goto [reg]          jumps to position in reg (error if not an int)
pair [n] [reg]      extends list in reg with constant #n as the head
head [reg]          replaces reg (a pair) with its head
tail [reg]          replaces reg (a pair) with its tail
push [reg1] [reg2]  extends list in reg2 with value in reg1 as the head
pop [reg1] [reg2]   puts the head of list in reg1 into reg2; replaces reg1 with its tail

lookup [n] [reg]    looks up constant #n (a symbol) in the env, and puts it in reg
define [n] [reg]    defines constant #n (a symbol) to the value in reg in the current env

not [reg]           if reg is false or nil, sets reg to true; otherwise sets reg to false
floor [reg]         sets reg to its mathematical floor (must be numeric)
print [reg]         prints the value in reg

equal [reg]         sets reg to true when arg1 and arg2 are equal; false otherwise
gt [reg]            sets reg to true when arg1 > arg2; false otherwise (args must be numeric)
lt [reg]            sets reg to true when arg1 < arg2; false otherwise (args must be numeric)
add [reg]           adds arg1 and arg2, result in reg (args must be numeric)
sub [reg]           subtracts arg1 and arg2, result in reg (args must be numeric)
mul [reg]           multiplies arg1 and arg2, result in reg (args must be numeric)
div [reg]           divides arg1 and arg2, result in reg (args must be numeric)

str [reg]           replaces reg (a symbol) with the binary symbol name
*/

typedef enum {
  OpNoop,
  OpHalt,
  OpConst,
  OpMove,
  OpBranch,
  OpJump,
  OpGoto,
  OpPair,
  OpHead,
  OpTail,
  OpPush,
  OpPop,
  OpLookup,
  OpDefine,

  OpNot,
  OpFloor,
  OpPrint,

  OpEqual,
  OpGt,
  OpLt,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,

  OpStr,

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
Val OpSymbol(OpCode op);
