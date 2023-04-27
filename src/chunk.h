#pragma once
#include "mem.h"

// #define DEBUG_ASSEMBLE



typedef enum {
  OpNoop,
  OpConst,
  OpLookup,
  OpDefine,
  OpBranch,
  OpNot,
  OpLambda,
  OpDefArg,
  OpExtEnv,
  OpPushArg,
  OpBrPrim,
  OpPrim,
  OpApply,
  OpMove,
  OpPush,
  OpPop,
  OpJump,
  OpReturn,
  OpHalt,

  NUM_OPCODES
} OpCode;

typedef struct {
  u8 *data;
  Val *constants;
} Chunk;

u8 ChunkRef(Chunk *chunk, u32 index);
Val ChunkConst(Chunk *chunk, u32 index);

u32 OpLength(OpCode op);

Chunk Assemble(Val stmts, Mem *mem);
void PrintChunk(Chunk *chunk, Mem *mem);
u32 PrintInstruction(Chunk *chunk, u32 i, Mem *mem);
void PrintChunkConstants(Chunk *chunk, Mem *mem);

u32 PrintOpCode(OpCode op);
