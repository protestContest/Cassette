#include "univ/iff.h"
#include "univ/str.h"
#include "univ/math.h"

struct IFFChunk {
  u32 type;
  u32 size;
};

static IFFChunk *IFFAppend(IFFChunk *chunk, void *data, i32 size)
{
  i32 pad = (IFFDataSize(chunk) + size) & 1;
  chunk = realloc(chunk, IFFChunkSize(chunk) + size + pad);
  Copy(data, (u8*)chunk + IFFChunkSize(chunk), size);
  chunk->size = ByteSwap(IFFDataSize(chunk) + size);
  if (pad) ((u8*)chunk)[IFFChunkSize(chunk) - 1] = 0;
  return chunk;
}

IFFChunk *NewIFFChunk(u32 type, void *data, i32 size)
{
  IFFChunk *chunk = malloc(sizeof(IFFChunk));
  chunk->type = ByteSwap(type);
  chunk->size = 0;
  return IFFAppend(chunk, data, size);
}

IFFChunk *NewIFFForm(u32 type)
{
  type = ByteSwap(type);
  return NewIFFChunk('FORM', &type, sizeof(type));
}

IFFChunk *IFFAppendChunk(IFFChunk *chunk, IFFChunk *child)
{
  return IFFAppend(chunk, child, IFFChunkSize(child));
}

IFFChunk *IFFGetFormField(IFFChunk *form, u32 type, u32 field)
{
  IFFChunk *chunk;
  IFFChunk *end = (IFFChunk*)((u8*)form + IFFChunkSize(form));
  if (IFFFormType(form) != type) return 0;

  chunk = (IFFChunk*)((u32*)IFFData(form)+1);
  while (chunk < end) {
    if (IFFChunkType(chunk) == 'FORM' && IFFFormType(chunk) == field) return chunk;
    if (IFFChunkType(chunk) == field) return chunk;
    chunk = (IFFChunk*)((u8*)chunk + IFFChunkSize(chunk));
  }

  return 0;
}

u32 IFFChunkType(IFFChunk *chunk)
{
  return ByteSwap(chunk->type);
}

u32 IFFFormType(IFFChunk *chunk)
{
  if (IFFChunkType(chunk) != 'FORM') return 0;
  return ByteSwap(*(u32*)IFFData(chunk));
}

u32 IFFDataSize(IFFChunk *chunk)
{
  return ByteSwap(chunk->size);
}

void *IFFData(IFFChunk *chunk)
{
  return chunk + 1;
}

u32 IFFChunkSize(IFFChunk *chunk)
{
  i32 pad = IFFDataSize(chunk) & 1;
  return sizeof(IFFChunk) + IFFDataSize(chunk) + pad;
}
