#include "chunk.h"
#include "ops.h"
#include "leb.h"
#include "univ/str.h"
#include <univ/vec.h>

Chunk *NewChunk(void)
{
  Chunk *chunk = malloc(sizeof(Chunk));
  chunk->data = 0;
  chunk->needs_env = false;
  chunk->modifies_env = false;
  chunk->next = 0;
  return chunk;
}

void FreeChunk(Chunk *chunk)
{
  if (chunk->data) FreeVec(chunk->data);
  if (chunk->next) FreeChunk(chunk->next);
  free(chunk);
}

void ChunkWrite(u8 byte, Chunk *chunk)
{
  while (chunk->next) chunk = chunk->next;
  VecPush(chunk->data, byte);
}

void ChunkWriteInt(u32 num, Chunk *chunk)
{
  u32 size = LEBSize(num);
  u32 index;
  while (chunk->next) chunk = chunk->next;
  index = VecCount(chunk->data);
  GrowVec(chunk->data, size);
  WriteLEB(num, index, chunk->data);
}

u32 ChunkSize(Chunk *chunk)
{
  u32 size = 0;
  while (chunk) {
    size += VecCount(chunk->data);
    chunk = chunk->next;
  }
  return size;
}

Chunk *AppendChunk(Chunk *first, Chunk *second)
{
  Chunk *last;
  if (!second) return first;
  if (!first) return second;
  last = first;
  while (last->next) last = last->next;
  last->next = second;
  first->needs_env =
      first->needs_env | (second->needs_env & ~first->modifies_env);
  first->modifies_env = first->modifies_env | second->modifies_env;
  return first;
}

void TackOnChunk(Chunk *first, Chunk *second)
{
  Chunk *last = first;
  while (last->next) last = last->next;
  last->next = second;
}

Chunk *PreservingEnv(Chunk *first, Chunk *second)
{
  if (!second) return first;
  if (!first) return second;
  if (second->needs_env && first->modifies_env) {
    Chunk *save_env = NewChunk();
    save_env->needs_env = true;
    save_env->modifies_env = false;
    ChunkWrite(opGetEnv, save_env);
    save_env->next = first;
    ChunkWrite(opSwap, first);
    ChunkWrite(opSetEnv, first);
    first = save_env;
  }
  return AppendChunk(first, second);
}

u8 *SerializeChunk(Chunk *chunk, u8 *dst)
{
  if (!dst) dst = NewVec(u8, ChunkSize(chunk));
  while (chunk) {
    Copy(chunk->data, dst, VecCount(chunk->data));
    dst += VecCount(chunk->data);
    chunk = chunk->next;
  }

  return dst;
}
