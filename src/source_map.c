#include "source_map.h"
#include "univ/symbol.h"
#include "univ/vec.h"

void InitSourceMap(SourceMap *map)
{
  InitVec(map->filenames);
  InitVec(map->file_map);
  InitVec(map->pos_map);
}

void DestroySourceMap(SourceMap *map)
{
  FreeVec(map->filenames);
  FreeVec(map->file_map);
  FreeVec(map->pos_map);
}

static u32 SerializeChunkSource(Chunk **chunk, SourceMap *map)
{
  u32 start = VecCount(map->pos_map);
  while (chunk) {
    u32 src = (*chunk)->src;
    u32 count = VecCount((*chunk)->data);
    chunk = (*chunk)->next;
    while (chunk && (*chunk)->src == src) {
      count += VecCount((*chunk)->data);
      chunk = (*chunk)->next;
    }
    VecPush(map->pos_map, src);
    VecPush(map->pos_map, count);
  }
  return VecCount(map->pos_map) - start;
}

void AddChunkSource(Chunk **chunk, char **filename, SourceMap *map)
{
  u32 filenum = VecCount(map->filenames);
  u32 name = 0;
  if (filename) {
    HLock(filename);
    name = Symbol(*filename);
    HUnlock(filename);
  }
  SerializeChunkSource(chunk, map);
  VecPush(map->filenames, name);
  VecPush(map->file_map, filenum);
  VecPush(map->file_map, ChunkSize(chunk));
}

u32 GetSourcePos(u32 code_index, SourceMap *map)
{
  u32 i = 0;
  u32 *cur = VecData(map->pos_map);
  while (cur < VecEnd(map->pos_map)) {
    if (i + cur[1] > code_index) return cur[0];
    i += cur[1];
    cur += 2;
  }
  return 0;
}

char **GetSourceFile(u32 code_index, SourceMap *map)
{
  u32 i = 0;
  u32 *cur = VecData(map->file_map);
  while (cur < VecEnd(map->file_map)) {
    if (i + cur[1] >= code_index) return SymbolName(VecAt(map->filenames, cur[0]));
    i += cur[1];
    cur += 2;
  }
  return 0;
}
