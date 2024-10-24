#pragma once

#include "chunk.h"

typedef struct {
  u32 *filenames; /* vec */
  u32 *file_map; /* vec */
  u32 *pos_map; /* vec */
} SourceMap;

void InitSourceMap(SourceMap *map);
void DestroySourceMap(SourceMap *map);
void AddChunkSource(Chunk *chunk, char *filename, SourceMap *map);
u32 GetSourcePos(u32 code_index, SourceMap *map);
char *GetSourceFile(u32 code_index, SourceMap *map);
