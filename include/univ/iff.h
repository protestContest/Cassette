#pragma once

/* Functions for the Interchange File Format. */

typedef struct IFFChunk IFFChunk;

/* Creates a new chunk with the given data */
IFFChunk *NewIFFChunk(u32 type, void *data, i32 size);

/* Creates a new, empty form chunk of the given type */
IFFChunk *NewIFFForm(u32 type);

/* Appends the child chunk to the end of a chunk's data */
IFFChunk *IFFAppendChunk(IFFChunk *chunk, IFFChunk *child);

/* Returns a field from a form chunk */
IFFChunk *IFFGetField(IFFChunk *form, u32 index);

/* Returns a chunk's type */
u32 IFFChunkType(IFFChunk *chunk);

/* Returns a form chunk's form type */
u32 IFFFormType(IFFChunk *chunk);

/* Returns the size of a chunk's data */
u32 IFFDataSize(IFFChunk *chunk);

/* Returns a pointer to a chunk's data */
void *IFFData(IFFChunk *chunk);

/* Returns the total size of a chunk */
u32 IFFChunkSize(IFFChunk *chunk);
