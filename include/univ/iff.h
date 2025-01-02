#pragma once

typedef struct IFFChunk IFFChunk;

IFFChunk *NewIFFChunk(u32 type, void *data, i32 size);
IFFChunk *NewIFFForm(u32 type);
IFFChunk *IFFAppendChunk(IFFChunk *chunk, IFFChunk *child);
IFFChunk *IFFGetField(IFFChunk *form, u32 index);

u32 IFFChunkType(IFFChunk *chunk);
u32 IFFFormType(IFFChunk *chunk);
u32 IFFDataSize(IFFChunk *chunk);
void *IFFData(IFFChunk *chunk);
u32 IFFChunkSize(IFFChunk *chunk);
