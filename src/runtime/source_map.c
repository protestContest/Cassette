#include "runtime/source_map.h"
#include "runtime/symbol.h"
#include "univ/vec.h"

void InitSourceMap(SourceMap *map)
{
  map->file_map = 0;
  map->pos_map = 0;
}

void DestroySourceMap(SourceMap *map)
{
  FreeVec(map->file_map);
  FreeVec(map->pos_map);
}

u32 GetSourcePos(u32 code_index, SourceMap *map)
{
  u32 i = 0;
  u32 *cur = map->pos_map;
  if (!cur) return 0;
  while (cur < VecEnd(map->pos_map)) {
    if (i + cur[1] > code_index) return cur[0];
    i += cur[1];
    cur += 2;
  }
  return 0;
}

char *GetSourceFile(u32 code_index, SourceMap *map)
{
  u32 i = 0;
  u32 *cur = map->file_map;
  if (!cur) return 0;
  while (cur < VecEnd(map->file_map)) {
    if (i + cur[1] >= code_index) return SymbolName(cur[0]);
    i += cur[1];
    cur += 2;
  }
  return 0;
}

void AddSourcePos(SourceMap *map, u32 src, u32 count)
{
  if (VecCount(map->pos_map) > 1) {
    u32 *cur = VecEnd(map->pos_map) - 2;
    if (cur[0] == src) {
      cur[1] += count;
      return;
    }
  }

  VecPush(map->pos_map, src);
  VecPush(map->pos_map, count);
}

void AddSourceFile(SourceMap *map, char *filename, u32 count)
{
  VecPush(map->file_map, filename ? Symbol(filename) : 0);
  VecPush(map->file_map, count);
}
