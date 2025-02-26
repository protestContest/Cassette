#pragma once

/*
 * A SourceMap maps bytecode addresses to source locations in files.
 *
 * `file_map` is a list of (filename, length) pairs, where filename is a symbol and length is how
 * many bytes of code it covers. Entries in file_map must be placed in order of appearance in the
 * bytecode. For segments of code that don't map to a file, 0 is used as the filename.
 *
 * `pos_map` is a list of (pos, length) pairs, where pos is the index in a source file and length is
 * how many bytes of code it covers. Entries in the pos_map must be placed in order of appearance in
 * the bytecode.
 */

typedef struct {
  u32 *file_map; /* vec */
  u32 *pos_map; /* vec */
} SourceMap;

void InitSourceMap(SourceMap *map);
void DestroySourceMap(SourceMap *map);
u32 GetSourcePos(u32 code_index, SourceMap *map);
char *GetSourceFile(u32 code_index, SourceMap *map);
void AddSourcePos(SourceMap *map, u32 src, u32 count);
void AddSourceFile(SourceMap *map, char *filename, u32 count);
