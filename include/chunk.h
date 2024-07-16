#pragma once

/* A chunk abstractly represents a sequence of VM instructions. Each chunk also
records which registers it needs and which it modifies. This allows the compiler
to correctly generate code that preserves registers. */

#include "mem.h"

typedef val Chunk;

typedef enum {regEnv = 1} Regs;

Chunk MakeChunk(Regs needs, Regs modifies, val code);
#define EmptyChunk()          MakeChunk(0, 0, 0)
#define ChunkNeeds(chunk)     RawInt(TupleGet(chunk, 1))
#define ChunkModifies(chunk)  RawInt(TupleGet(chunk, 2))
#define ChunkCode(chunk)      TupleGet(chunk, 3)

#define Op(name)        SymVal(Symbol(name))

Chunk AppendChunk(Chunk a, Chunk b);
Chunk Preserving(Regs regs, Chunk a, Chunk b);
Chunk AppendCode(Chunk a, val code);
Chunk ParallelChunks(Chunk a, Chunk b);
Chunk TackOnChunk(Chunk a, Chunk b);
Chunk LabelChunk(val label, Chunk chunk);
void PrintChunk(Chunk chunk);
