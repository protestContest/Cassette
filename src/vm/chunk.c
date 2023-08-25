#include "chunk.h"

void InitChunk(Chunk *chunk)
{
  chunk->data = NULL;
  InitMem(&chunk->constants, 0);
  chunk->num_modules = 0;
}

void DestroyChunk(Chunk *chunk)
{
  FreeVec(chunk->data);
  DestroyMem(&chunk->constants);
}

u32 ChunkSize(Chunk *chunk)
{
  return VecCount(chunk->data);
}

u8 ChunkRef(Chunk *chunk, u32 i)
{
  return chunk->data[i];
}

Val ChunkConst(Chunk *chunk, u32 i)
{
  return chunk->constants.values[chunk->data[i]];
}

Val *ChunkBinary(Chunk *chunk, u32 i)
{
  u32 index = RawInt(ChunkConst(chunk, i));
  return chunk->constants.values + index;
}

void PushByte(u8 byte, Chunk *chunk)
{
  VecPush(chunk->data, byte);
}

void PushConst(Val value, Chunk *chunk)
{
  u32 index = VecCount(chunk->constants.values);
  VecPush(chunk->constants.values, value);
  VecPush(chunk->data, index);
}

u8 *SerializeChunk(Chunk *chunk)
{
  u32 size = VecCount(chunk->data) + VecCount(chunk->constants.values)*sizeof(Val) + 2*sizeof(u32);
  u8 *serialized = NewVec(u8, size);

  u8 *cur = serialized;
  u32 data_size = VecCount(chunk->data);
  u32 num_consts = VecCount(chunk->constants.values);

  Copy(&data_size, cur, sizeof(data_size));
  cur += sizeof(data_size);
  Copy(chunk->data, cur, data_size);
  cur += data_size;
  Copy(&num_consts, cur, sizeof(num_consts));
  cur += sizeof(num_consts);
  Copy(chunk->constants.values, cur, num_consts*sizeof(Val));
  RawVecCount(serialized) = size;

  return serialized;
}

void DeserializeChunk(u8 *serialized, Chunk *chunk)
{
  u32 data_size, num_consts;
  u8 *cur = serialized;

  Copy(cur, &data_size, sizeof(data_size));
  cur += sizeof(data_size);
  chunk->data = NewVec(u8, data_size);
  Copy(cur, chunk->data, data_size);
  cur += data_size;
  RawVecCount(chunk->data) = data_size;

  Copy(cur, &num_consts, sizeof(num_consts));
  cur += sizeof(num_consts);
  chunk->constants.values = NewVec(Val, num_consts);
  Copy(cur, chunk->constants.values, num_consts*sizeof(Val));
  RawVecCount(chunk->constants.values) = num_consts;
}
