#include "chunk.h"
#include "printer.h"
#include "vec.h"

typedef struct {
  char *name;
  ArgFormat format;
} OpInfo;

static OpInfo ops[] = {
  [OP_RETURN] =   { "return", ARGS_NONE     },
  [OP_CONST] =    { "const",  ARGS_VAL      },
  // [OP_LABEL] =    { OP_LABEL,   "label ", ARGS_VAL      },
  // [OP_MOVE] =     { OP_MOVE,    "move ",  ARGS_REG_REG  },
  // [OP_ASSIGN] =   { OP_ASSIGN,  "assign", ARGS_VAL_REG  },
  // [OP_TEST] =     { OP_TEST,    "test  ", ARGS_VAL_REG  },
  // [OP_BRANCH] =   { OP_BRANCH,  "branch", ARGS_VAL      },
  // [OP_GOTO] =     { OP_BRANCH,  "goto  ", ARGS_VAL      },
  // [OP_SAVE] =     { OP_SAVE,    "push  ", ARGS_REG      },
  // [OP_RESTORE] =  { OP_RESTORE, "pop   ", ARGS_REG      },
  // [OP_ADD] =      { OP_ADD,     "add   ", ARGS_VAL_REG  },
};

const char *OpStr(OpCode op)
{
  return ops[op].name;
}

ArgFormat OpFormat(OpCode op)
{
  return ops[op].format;
}

u32 OpSize(OpCode op)
{
  switch (OpFormat(op)) {
  case ARGS_NONE:     return 1;
  case ARGS_VAL:      return 2;
  }
}

Chunk *NewChunk(void)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  if (!chunk) Error("Out of memory");
  return chunk;
}

Chunk *InitChunk(Chunk *chunk)
{
  chunk->code = NULL;
  chunk->lines = NULL;
  chunk->constants = NULL;
  return chunk;
}

void FreeChunk(Chunk *chunk)
{
  FreeVec(chunk->code);
  FreeVec(chunk->lines);
  FreeVec(chunk->constants);
}

u32 PutByte(Chunk *chunk, u32 line, u8 byte)
{
  VecPush(chunk->code, byte);
  VecPush(chunk->lines, line);
  return VecCount(chunk->code) - 1;
}

u8 GetByte(Chunk *chunk, u32 i)
{
  return chunk->code[i];
}


u8 PutConst(Chunk *chunk, Val value)
{
  VecPush(chunk->constants, value);
  return VecCount(chunk->constants) - 1;
}

Val GetConst(Chunk *chunk, u32 i)
{
  return chunk->constants[i];
}

u32 PutInst(Chunk *chunk, u32 line, OpCode op, ...)
{
  va_list args;
  va_start(args, op);

  PutByte(chunk, line, op);

  switch (op) {
  case OP_RETURN:
    break;

  case OP_CONST:
    PutByte(chunk, line, PutConst(chunk, va_arg(args, Val)));
    break;

  }

  va_end(args);
  return VecCount(chunk->code) - OpSize(op);
}

void DisassembleInstruction(Chunk *chunk, u32 i)
{
  printf("%4u  ", i);
  if (i > 0 && chunk->lines[i] == chunk->lines[i - 1]) {
    printf("   │ ");
  } else {
    printf("%3u│ ", chunk->lines[i]);
  }

  OpCode op = chunk->code[i];
  switch (OpFormat(op)) {
  case ARGS_NONE:
    printf("%s", OpStr(op));
    break;
  case ARGS_VAL: {
    printf("%s %s", OpStr(op), ValStr(GetConst(chunk, GetByte(chunk, i + 1))));
    break;
  }
  }
}

void Disassemble(char *title, Chunk *chunk)
{
  printf(" ━━╸%s╺━━\n", title);

  for (u32 i = 0; i < VecCount(chunk->code); i += OpSize(chunk->code[i])) {
    DisassembleInstruction(chunk, i);
    printf("\n");
  }
  printf("\n");
}
