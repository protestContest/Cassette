#pragma once

/* A chunk is a linked list, where each node has a snippet of bytecode. A chunk
 * also keeps track of whether it (or any chunk after it) needs or modifies the
 * env register.
 */

typedef struct Chunk {
  u8 *data;
  bool needs_env;
  bool modifies_env;
  struct Chunk *next;
} Chunk;

Chunk *NewChunk(void);
void FreeChunk(Chunk *chunk);
void ChunkMakeRoom(u32 size, Chunk *chunk);
void ChunkWrite(u8 byte, Chunk *chunk);
void ChunkWriteInt(u32 num, Chunk *chunk);
u32 ChunkSize(Chunk *chunk);
Chunk *AppendChunk(Chunk *first, Chunk *second);
void TackOnChunk(Chunk *first, Chunk *second);
Chunk *PreservingEnv(Chunk *first, Chunk *second);
u8 *SerializeChunk(Chunk *chunk, u8 *dst);