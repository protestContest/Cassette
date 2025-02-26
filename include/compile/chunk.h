#pragma once

/*
 * A chunk is a sequence of bytecode. It also keeps track of whether the chunk needs or modifies the
 * env register, and its source file position.
 *
 * A chunk is a linked list, but logically represents the code in the entire list. `needs_env` and
 * `modifies_env` should represent all chunks, and functions to append or emit chunks work on chunk
 * lists as a whole.
 */

typedef struct Chunk {
  u8 *data; /* vec */
  bool needs_env;
  bool modifies_env;
  u32 src;
  struct Chunk *next;
} Chunk;

Chunk *NewChunk(u32 src);
void FreeChunk(Chunk *chunk);
void Emit(u8 byte, Chunk *chunk);
void EmitInt(u32 num, Chunk *chunk);
u32 ChunkSize(Chunk *chunk);
Chunk *PrependChunk(u8 byte, Chunk *chunk);
Chunk *AppendChunk(Chunk *first, Chunk *second);
void TackOnChunk(Chunk *first, Chunk *second);
Chunk *PreservingEnv(Chunk *first, Chunk *second);
Chunk *ParallelChunks(Chunk *first, Chunk *second);
u8 *SerializeChunk(Chunk *chunk, u8 *dst);

#ifdef DEBUG
void DisassembleChunk(Chunk *chunk);
#endif
