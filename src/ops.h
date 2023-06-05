#pragma once
#include "mem.h"

/*
Op codes

-- control flow --
halt                    halts execution
branch [n] [reg]        jumps to position n when reg is not false or nil
branchf [n] [reg]       jumps to position n when reg is false or nil
jump [n]                jumps to position n
goto [reg]              jumps to position in reg (error if not an int)

-- registers --
const [n] [reg]         loads constant #n into register reg
move [reg1] [reg2]      copies value from reg1 to reg2

-- memory --
str [n] [reg]           make a string from the symbol constant #n, put it in reg
lambda [n] [reg]        make a function with entrypoint at n and the current env, put it in reg
pair [n] [reg]          extends list in reg with constant #n as the head
tuple [n] [reg]         replaces reg with a tuple of length n
tset [reg1] [n] [reg2]  for a tuple in reg1, sets tuple index n to value in reg2 (reg1 must be a tuple)
map [reg]               replaces reg with a map made from the keys/values pair of tuples in reg (reg must be a pair of tuples)
head [reg]              replaces reg (a pair) with its head
tail [reg]              replaces reg (a pair) with its tail
push [reg1] [reg2]      extends list in reg2 with value in reg1 as the head
pop [reg1] [reg2]       puts the head of list in reg1 into reg2; replaces reg1 with its tail

-- environment --
lookup [n] [m] [reg]    looks up a variable in the env at frame n, entry m, and puts it in reg
define [n] [reg]        defines constant #n (a symbol) to the value in reg in the current env

-- logic/arithmetic --
not [reg]               if reg is false or nil, sets reg to true; otherwise sets reg to false
equal [reg]             sets reg to true when arg1 and arg2 are equal; false otherwise
gt [reg]                sets reg to true when arg1 > arg2; false otherwise (args must be numeric)
lt [reg]                sets reg to true when arg1 < arg2; false otherwise (args must be numeric)
add [reg]               adds arg1 and arg2, result in reg (args must be numeric)
sub [reg]               subtracts arg1 and arg2, result in reg (args must be numeric)
mul [reg]               multiplies arg1 and arg2, result in reg (args must be numeric)
div [reg]               divides arg1 and arg2, result in reg (args must be numeric)

-- escape hatch --
prim [reg]              given a symbol in RegFun and arguments in RegArgs, calls a primitive procedure and puts the result in reg
*/

typedef enum {
  OpNoop,
  OpHalt,
  OpConst,
  OpMove,
  OpBranch,
  OpBranchF,
  OpJump,
  OpGoto,
  OpStr,
  OpLambda,
  OpPair,
  OpTuple,
  OpTSet,
  OpMap,
  OpHead,
  OpTail,
  OpPush,
  OpPop,
  OpLookup,
  OpDefine,
  OpPrim,
  OpNot,
  OpEqual,
  OpGt,
  OpLt,
  OpAdd,
  OpSub,
  OpMul,
  OpDiv,

  NUM_OPCODES
} OpCode;

typedef enum {
  ArgsNone,
  ArgsConst,
  ArgsReg,
  ArgsConstReg,
  ArgsRegReg,
  ArgsConstConstReg,
  ArgsRegConstReg,
} ArgInfo;

char *OpName(OpCode op);
ArgInfo OpArgs(OpCode op);
u32 OpLength(OpCode op);
void InitOps(Mem *mem);
u32 PrintOpCode(OpCode op);
Val OpSymbol(OpCode op);
OpCode OpForSymbol(Val sym);
