#include "chunk.h"
#include "rec.h"
#include "univ/memory.h"
#include "univ/math.h"
#include "univ/string.h"
#include "univ/system.h"
#include "univ/file.h"

void InitChunk(Chunk *chunk)
{
  chunk->capacity = 256;
  chunk->count = 0;
  chunk->data = Allocate(chunk->capacity);
  InitMem(&chunk->constants, 0);
  chunk->source_map = EmptyHashMap;
  chunk->sources = NULL;
}

void DestroyChunk(Chunk *chunk)
{
  Free(chunk->data);
  DestroyMem(&chunk->constants);
  DestroyHashMap(&chunk->source_map);
  Free(chunk->sources);
}

void FreeChunk(Chunk *chunk)
{
  DestroyChunk(chunk);
  Free(chunk);
}

u32 ChunkSize(Chunk *chunk)
{
  return chunk->count;
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
  if (chunk->count >= chunk->capacity) {
    chunk->capacity = Max(256, chunk->capacity*2);
    chunk->data = Reallocate(chunk->data, chunk->capacity);
  }
  chunk->data[chunk->count++] = byte;
}

static u32 FindConst(Val value, Chunk *chunk)
{
  u32 count = MemSize(&chunk->constants);
  for (u32 i = 0; i < count; i++) {
    if (Eq(value, chunk->constants.values[i])) return i;
  }
  return count;
}

void PushConst(Val value, Chunk *chunk)
{
  u32 index = FindConst(value, chunk);
  Assert(index < 256);

  if (index == MemSize(&chunk->constants)) {
    PushVal(&chunk->constants, value);
  }
  PushByte(index, chunk);
}

void AddSourceFile(char *filename, u32 offset, Chunk *chunk)
{
  if (chunk->sources_count >= chunk->sources_capacity) {
    chunk->sources_capacity = Max(2, 2*chunk->sources_capacity);
    chunk->sources = Reallocate(chunk->sources, sizeof(FileOffset)*chunk->sources_capacity);
  }
  FileOffset *entry = &chunk->sources[chunk->sources_count++];

  u32 len = StrLen(filename);
  entry->filename = Allocate(len + 1);
  Copy(filename, entry->filename, len);
  entry->filename[len] = '\0';
  entry->offset = offset;
}

char *SourceFile(u32 pos, Chunk *chunk)
{
  char *filename = NULL;
  for (u32 i = 0; i < chunk->sources_count; i++) {
    if (chunk->sources[i].offset > pos) break;
    filename = chunk->sources[i].filename;
  }

  return filename;
}

u32 SourcePos(u32 chunk_pos, Chunk *chunk)
{
  while (true) {
    if (HashMapContains(&chunk->source_map, chunk_pos)) {
      return HashMapGet(&chunk->source_map, chunk_pos);
    } else if (chunk_pos == 0) {
      return 0;
    } else {
      chunk_pos--;
    }
  }
}

u32 ChunkTag(void)
{
  // CA55E77E
  return (0x7E << 24) | (0xE7 << 16) | (0x55 << 8) | (0xCA);
}

u32 SerializedSize(Chunk *chunk)
{
  u32 tag = ChunkTag();
  u32 version = CurrentVersion;
  u32 code_size = chunk->count;
  u32 const_size = MemSize(&chunk->constants)*sizeof(Val);
  u32 strings_size = chunk->constants.strings_count;
  u32 string_map_size = chunk->constants.string_map.count*sizeof(u32)*2;

  return
    sizeof(tag) +
    sizeof(version) +
    sizeof(code_size) +
    code_size +
    sizeof(const_size) +
    const_size +
    sizeof(strings_size) +
    strings_size +
    sizeof(string_map_size) +
    string_map_size;
}

u8 *SerializeChunk(Chunk *chunk)
{
  u32 tag = ChunkTag();
  u32 version = CurrentVersion;
  u32 code_size = chunk->count;
  u32 const_size = MemSize(&chunk->constants)*sizeof(Val);
  u32 strings_size = chunk->constants.strings_count;
  u32 string_map_size = chunk->constants.string_map.count*sizeof(u32)*2;

  u32 size =
    sizeof(tag) +
    sizeof(version) +
    sizeof(code_size) +
    code_size +
    sizeof(const_size) +
    const_size +
    sizeof(strings_size) +
    strings_size +
    sizeof(string_map_size) +
    string_map_size;

  u8 *serialized = Allocate(size);
  u8 *cur = serialized;

  Copy(&tag, cur, sizeof(tag));
  cur += sizeof(tag);
  Copy(&version, cur, sizeof(version));
  cur += sizeof(version);
  Copy(&code_size, cur, sizeof(code_size));
  cur += sizeof(code_size);
  Copy(chunk->data, cur, code_size);
  cur += code_size;
  Copy(&const_size, cur, sizeof(const_size));
  cur += sizeof(const_size);
  Copy(chunk->constants.values, cur, const_size);
  cur += const_size;
  Copy(&strings_size, cur, sizeof(strings_size));
  cur += sizeof(strings_size);
  Copy(chunk->constants.strings, cur, strings_size);
  cur += strings_size;
  Copy(&string_map_size, cur, sizeof(string_map_size));
  cur += sizeof(string_map_size);

  for (u32 i = 0; i < chunk->constants.string_map.count; i++) {
    u32 key = HashMapKey(&chunk->constants.string_map, i);
    u32 value = HashMapGet(&chunk->constants.string_map, key);

    Copy(&key, cur, sizeof(key));
    cur += sizeof(key);
    Copy(&value, cur, sizeof(value));
    cur += sizeof(value);
  }

  return serialized;
}

void DeserializeChunk(u8 *serialized, Chunk *chunk)
{
  u32 version, code_size, const_size, strings_size, string_map_size;

  u8 *cur = serialized + sizeof(u32); // skip tag
  Copy(cur, &version, sizeof(version));
  if (version != CurrentVersion) return;
  Copy(cur, &code_size, sizeof(code_size));
  cur += sizeof(code_size);
  chunk->data = Allocate(code_size);
  chunk->count = code_size;
  chunk->capacity = code_size;
  Copy(cur, chunk->data, code_size);
  cur += code_size;
  Copy(cur, &const_size, sizeof(const_size));
  cur += sizeof(const_size);
  chunk->constants.values = Allocate(const_size);
  chunk->constants.count = const_size/sizeof(Val);
  chunk->constants.capacity = const_size/sizeof(Val);
  Copy(cur, chunk->constants.values, const_size);
  cur += const_size;
  Copy(cur, &strings_size, sizeof(strings_size));
  cur += sizeof(strings_size);
  chunk->constants.strings = Allocate(strings_size);
  chunk->constants.strings_capacity = strings_size;
  chunk->constants.strings_count = strings_size;
  Copy(cur, chunk->constants.strings, strings_size);
  cur += strings_size;
  Copy(cur, &string_map_size, sizeof(string_map_size));
  cur += sizeof(string_map_size);

  chunk->constants.string_map = EmptyHashMap;
  for (u32 i = 0; i < string_map_size/(2*sizeof(u32)); i++) {
    u32 key, value;
    Copy(cur, &key, sizeof(key));
    cur += sizeof(key);
    Copy(cur, &value, sizeof(value));
    cur += sizeof(value);

    HashMapSet(&chunk->constants.string_map, key, value);
  }
}

static Chunk *ReadChunk(char *path)
{
  Chunk *chunk = Allocate(sizeof(Chunk));
  InitChunk(chunk);

  u8 *file = ReadFile(path);
  if (!file) return NULL;

  DeserializeChunk(file, chunk);

  return chunk;
}

static Chunk *CompileChunk(CassetteOpts *opts, Heap *mem)
{
  Chunk *chunk = Allocate(sizeof(Chunk));
  InitChunk(chunk);

  SeedPrimitives();

  // compile project
  CompileResult result = LoadModules(opts, mem);
  if (!result.ok) {
    return NULL;
  }

  Assemble(result.code, chunk, mem);

  return chunk;
}

Chunk *LoadChunk(CassetteOpts *opts)
{
  Chunk *chunk = NULL;

  Heap mem;
  InitMem(&mem, 0);

  if (SniffFile(opts->entry, ChunkTag())) {
    // read compiled chunk from file
    chunk = ReadChunk(opts->entry);
  } else {
    // compile project into a chunk
    chunk = CompileChunk(opts, &mem);
  }
  DestroyMem(&mem);

  return chunk;
}
