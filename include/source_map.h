#pragma once
#include "chunk.h"
#include "univ/vec.h"

typedef struct {
  WordVec(filenames);
  WordVec(file_map);
  WordVec(pos_map);
} SourceMap;

void InitSourceMap(SourceMap *map);
void DestroySourceMap(SourceMap *map);
void AddChunkSource(Chunk **chunk, char **filename, SourceMap *map);
u32 GetSourcePos(u32 code_index, SourceMap *map);
char **GetSourceFile(u32 code_index, SourceMap *map);
