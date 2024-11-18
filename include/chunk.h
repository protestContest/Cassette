#pragma once
#include "univ/vec.h"

/* A chunk is a linked list, where each node has a snippet of bytecode. A chunk
 * also keeps track of whether it (or any chunk after it) needs or modifies the
 * env register.
 */

typedef struct Chunk {
  VecOf(u8) **data;
  bool needs_env;
  bool modifies_env;
  u32 src;
  struct Chunk **next;
} Chunk;

Chunk **NewChunk(u32 src);
void FreeChunk(Chunk **chunk);
void ChunkWrite(u8 byte, Chunk **chunk);
void ChunkWriteInt(u32 num, Chunk **chunk);
u32 ChunkSize(Chunk **chunk);
Chunk **PrependChunk(u8 byte, Chunk **chunk);
Chunk **AppendChunk(Chunk **first, Chunk **second);
void TackOnChunk(Chunk **first, Chunk **second);
Chunk **PreservingEnv(Chunk **first, Chunk **second);
Chunk **ParallelChunks(Chunk **first, Chunk **second);
u8 *SerializeChunk(Chunk **chunk, u8 *dst);

#ifdef DEBUG
void DisassembleChunk(Chunk **chunk);
#endif
