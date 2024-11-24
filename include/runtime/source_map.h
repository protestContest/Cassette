#pragma once

typedef struct {
  u32 *filenames; /* vec */
  u32 *file_map; /* vec */
  u32 *pos_map; /* vec */
} SourceMap;

void InitSourceMap(SourceMap *map);
void DestroySourceMap(SourceMap *map);
u32 GetSourcePos(u32 code_index, SourceMap *map);
char *GetSourceFile(u32 code_index, SourceMap *map);
void AddSourcePos(SourceMap *map, u32 src, u32 count);
void AddSourceFile(SourceMap *map, char *filename, u32 count);
