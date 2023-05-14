#include "ops.h"

typedef struct {
  char *name;
  ArgInfo args;
} OpInfo;

static OpInfo op_info[NUM_OPCODES] = {
  [OpNoop]    = { "noop",     ArgsNone      },
  [OpHalt]    = { "halt",     ArgsNone      },
  [OpConst]   = { "const",    ArgsConstReg  },
  [OpMove]    = { "move",     ArgsRegReg    },
  [OpBranch]  = { "branch",   ArgsConstReg  },
  [OpBranchF] = { "branchf",  ArgsConstReg  },
  [OpJump]    = { "jump",     ArgsConst     },
  [OpGoto]    = { "goto",     ArgsReg       },
  [OpPair]    = { "pair",     ArgsConstReg  },
  [OpHead]    = { "head",     ArgsReg       },
  [OpTail]    = { "tail",     ArgsReg       },
  [OpPush]    = { "push",     ArgsRegReg    },
  [OpPop]     = { "pop",      ArgsRegReg    },
  [OpLookup]  = { "lookup",   ArgsConstReg  },
  [OpLookup2] = { "lookup2",  ArgsConstConstReg  },
  [OpDefine]  = { "define",   ArgsConstReg  },
  [OpNot]     = { "not",      ArgsReg       },
  [OpFloor]   = { "floor",    ArgsReg       },
  [OpPrint]   = { "print",    ArgsReg       },
  [OpEqual]   = { "equal",    ArgsReg       },
  [OpGt]      = { "gt",       ArgsReg       },
  [OpLt]      = { "lt",       ArgsReg       },
  [OpAdd]     = { "add",      ArgsReg       },
  [OpSub]     = { "sub",      ArgsReg       },
  [OpMul]     = { "mul",      ArgsReg       },
  [OpDiv]     = { "div",      ArgsReg       },
  [OpStr]     = { "str",      ArgsReg       },
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
  case ArgsConstConstReg:  return 4;
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

Val OpSymbol(OpCode op)
{
  return SymbolFor(op_info[op].name);
}

OpCode OpForSymbol(Val sym)
{
  for (u32 i = 0; i < ArrayCount(op_info); i++) {
    if (Eq(sym, OpSymbol(i))) {
      return i;
    }
  }
  return OpNoop;
}
