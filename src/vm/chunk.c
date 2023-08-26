#include "chunk.h"
#include "rec.h"

void InitChunk(Chunk *chunk)
{
  chunk->data = NULL;
  InitMem(&chunk->constants, 0);
  chunk->num_modules = 0;
}

void DestroyChunk(Chunk *chunk)
{
  FreeVec(chunk->data);
  DestroyMem(&chunk->constants);
}

void FreeChunk(Chunk *chunk)
{
  DestroyChunk(chunk);
  Free(chunk);
}

u32 ChunkSize(Chunk *chunk)
{
  return VecCount(chunk->data);
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
  VecPush(chunk->data, byte);
}

void PushConst(Val value, Chunk *chunk)
{
  u32 index = VecCount(chunk->constants.values);
  VecPush(chunk->constants.values, value);
  VecPush(chunk->data, index);
}

u32 ChunkTag(void)
{
  // CA55E77E
  return (0x7E << 24) | (0xE7 << 16) | (0x55 << 8) | (0xCA);
}

u8 *SerializeChunk(Chunk *chunk)
{
  u32 tag = ChunkTag();
  u32 code_size = VecCount(chunk->data);
  u32 const_size = MemSize(&chunk->constants)*sizeof(Val);
  u32 strings_size = VecCount(chunk->constants.strings);
  u32 string_map_size = chunk->constants.string_map.count*sizeof(u32)*2;

  u32 size =
    sizeof(tag) + // tag
    sizeof(u32) + // num modules
    sizeof(u32) + // size of code
    code_size + // code
    sizeof(u32) + // size of constants
    const_size + // constants
    sizeof(u32) + // size of strings
    strings_size + // strings
    sizeof(u32) + // size of string map
    string_map_size; // string map

  u8 *serialized = NewVec(u8, size);
  u8 *cur = serialized;

  Copy(&tag, cur, sizeof(tag));
  cur += sizeof(tag);
  Copy(&chunk->num_modules, cur, sizeof(chunk->num_modules));
  cur += sizeof(chunk->num_modules);
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

  RawVecCount(serialized) = size;

  return serialized;
}

void DeserializeChunk(u8 *serialized, Chunk *chunk)
{
  u32 code_size, const_size, strings_size, string_map_size;

  u8 *cur = serialized + sizeof(u32); // skip tag
  Copy(cur, &chunk->num_modules, sizeof(chunk->num_modules));
  cur += sizeof(chunk->num_modules);
  Copy(cur, &code_size, sizeof(code_size));
  cur += sizeof(code_size);
  chunk->data = NewVec(u8, code_size);
  RawVecCount(chunk->data) = code_size;
  Copy(cur, chunk->data, code_size);
  cur += code_size;
  Copy(cur, &const_size, sizeof(const_size));
  cur += sizeof(const_size);
  chunk->constants.values = NewVec(Val, const_size/sizeof(Val));
  RawVecCount(chunk->constants.values) = const_size/sizeof(Val);
  Copy(cur, chunk->constants.values, const_size);
  cur += const_size;
  Copy(cur, &strings_size, sizeof(strings_size));
  cur += sizeof(strings_size);
  chunk->constants.strings = NewVec(char, strings_size);
  RawVecCount(chunk->constants.strings) = strings_size;
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

Chunk *LoadChunk(char *path)
{
  Chunk *chunk = Allocate(sizeof(Chunk));
  InitChunk(chunk);

  u8 *file = ReadFile(path);
  if (!file) return NULL;

  DeserializeChunk(file, chunk);

  return chunk;
}

Chunk *CompileChunk(Args *args, Heap *mem)
{
  Chunk *chunk = Allocate(sizeof(Chunk));
  InitChunk(chunk);

  ModuleResult result = LoadModules(args, mem);
  if (!result.ok) {
    Print(result.error.message);
    Print(": ");
    Inspect(result.error.expr, mem);
    Print("\n");
    return NULL;
  }

  if (args->verbose) {
    PrintSeq(result.module.code, mem);
  }

  Assemble(result.module.code, chunk, mem);

  return chunk;
}
