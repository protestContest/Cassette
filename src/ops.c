#include "ops.h"

typedef struct {
  char *name;
  ArgInfo args;
} OpInfo;

static OpInfo op_info[NUM_OPCODES] = {
  [OpNoop] =    { "noop",     ArgsNone },
  [OpHalt] =    { "halt",     ArgsNone },
  [OpConst] =   { "const",    ArgsConstReg },
  [OpTest] =    { "test",     ArgsReg },
  [OpNot] =     { "not",      ArgsReg },
  [OpBranch] =  { "branch",   ArgsConst },
  [OpJump] =    { "jump",     ArgsConst },
  [OpGoto] =    { "goto",     ArgsReg },
  [OpPush] =    { "push",     ArgsRegReg },
  [OpPop] =     { "pop",      ArgsRegReg },
};

char *OpName(OpCode op)
{
  return op_info[op].name;
}

ArgInfo OpArgs(OpCode op)
{
  return op_info[op].args;
}

u32 OpLength(OpCode op)
{
  switch (op_info[op].args) {
  case ArgsNone:      return 1;
  case ArgsConst:     return 2;
  case ArgsReg:       return 2;
  case ArgsConstReg:  return 3;
  case ArgsRegReg:    return 3;
  }
}

void InitOps(Mem *mem)
{
  for (u32 i = 0; i < ArrayCount(op_info); i++) {
    MakeSymbol(mem, op_info[i].name);
  }
}

u32 PrintOpCode(OpCode op)
{
  return Print(OpName(op));
}
