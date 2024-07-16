#pragma once
#include "mem.h"

typedef enum {regEnv = 1, regCont = 2} Regs;

val MakeChunk(Regs needs, Regs modifies, val code);
#define EmptyChunk()          MakeChunk(0, 0, 0)
#define ChunkNeeds(chunk)     RawInt(TupleGet(chunk, 1))
#define ChunkModifies(chunk)  RawInt(TupleGet(chunk, 2))
#define ChunkCode(chunk)      TupleGet(chunk, 3)

#define Op(name)        SymVal(Symbol(name))

val AppendChunk(val a, val b);
val AppendChunks(val chunks);
val Preserving(i32 regs, val a, val b);
val AppendCode(val a, val code);
val ParallelChunks(val a, val b);
val TackOnChunk(val a, val b);
val LabelChunk(val label, val chunk);
void PrintChunk(val chunk);
