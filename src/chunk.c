#include "chunk.h"
#include "vm.h"
#include "ops.h"

void InitChunk(Chunk *chunk)
{
  chunk->data = NULL;
  InitMem(&chunk->constants);
}

void DestroyChunk(Chunk *chunk)
{
  DestroyMem(&chunk->constants);
  FreeVec(chunk->data);
}

u8 PushConst(Chunk *chunk, Val value)
{
  for (u32 i = 0; i < VecCount(chunk->constants.values); i++) {
    if (Eq(chunk->constants.values[i], value)) return i;
  }
  u32 n = VecCount(chunk->constants.values);
  VecPush(chunk->constants.values, value);
  return n;
}

void PushByte(Chunk *chunk, u8 byte)
{
  VecPush(chunk->data, byte);
}

void AddSymbol(Chunk *chunk, Val symbol, Mem *mem)
{
  char *name = SymbolName(symbol, mem);
  MakeSymbol(name, &chunk->constants);
}

void Disassemble(Chunk *chunk)
{
  Print("────┬─Chunk─\n");
  for (u32 i = 0; i < VecCount(chunk->data);) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    PrintInstruction(chunk, i);
    Print("\n");
    i += OpLength(chunk->data[i]);
  }
}

typedef struct {
  char id[4];
  u32 version;
  u32 string_offset;
  u32 constant_offset;
  u32 code_offset;
  u32 chunk_size;
} ChunkFileHeader;

static ChunkFileHeader ChunkSize(Chunk *chunk)
{
  ChunkFileHeader header;
  header.id[0] = 0xCA;
  header.id[1] = 0x55;
  header.id[2] = 0xE7;
  header.id[3] = 0x7E;
  header.version = 1;

  u32 size = 0;
  u32 num_strings = MapCount(&chunk->constants.string_map);
  size += sizeof(u32)*2*num_strings;
  header.string_offset = size;

  size += VecCount(chunk->constants.strings);
  header.constant_offset = size;

  size += sizeof(Val)*VecCount(chunk->constants.values);
  header.code_offset = size;

  size += VecCount(chunk->data);
  header.chunk_size = size;

  return header;
}

bool WriteChunk(Chunk *chunk, char *filename)
{
/*
Format:
- Magic number, 0xCA55E77E
- u32 position of string data
- u32 position of constants
- u32 position of code
- u32 chunk size
- string map: list of (u32 key, u32 value) pairs
- string data
- constants
- code
*/

  ChunkFileHeader header = ChunkSize(chunk);

  u8 file_data[header.chunk_size];
  u32 cur = 0;

  u32 num_strings = MapCount(&chunk->constants.string_map);
  for (u32 i = 0; i < num_strings; i++) {
    u32 key = GetMapKey(&chunk->constants.string_map, i);
    u32 val = GetMapValue(&chunk->constants.string_map, i);

    *((u32*)(file_data+cur)) = key;
    cur += sizeof(key);
    *((u32*)(file_data+cur)) = val;
    cur += sizeof(val);
  }

  Assert(cur == header.string_offset);

  for (u32 i = 0; i < VecCount(chunk->constants.strings); i++, cur++) {
    file_data[cur] = chunk->constants.strings[i];
  }

  Assert(cur == header.constant_offset);

  for (u32 i = 0; i < VecCount(chunk->constants.values); i++, cur += sizeof(Val)) {
    *((Val *)(file_data + cur)) = chunk->constants.values[i];
  }

  Assert(cur == header.code_offset);

  for (u32 i = 0; i < VecCount(chunk->data); i++, cur++) {
    file_data[cur] = chunk->data[i];
  }

  Assert(cur == header.chunk_size);

  HexDump(file_data, header.chunk_size);

  u8 output[header.chunk_size + sizeof(header)];
  *((ChunkFileHeader*)output) = header;
  u8 *compressed = output + sizeof(header);
  Compress(file_data, compressed, header.chunk_size);

  return WriteFile(filename, output, sizeof(header) + header.chunk_size);
}

bool ReadChunk(Chunk *chunk, char *filename)
{
  char *data = (char*)ReadFile(filename);
  if (data == NULL) return false;

  ChunkFileHeader header = *((ChunkFileHeader*)data);
  data += sizeof(header);

  char *strings = data + header.string_offset;
  char *constants = data + header.constant_offset;
  char *code = data + header.code_offset;
  char *end = data + header.chunk_size;

  InitMap(&chunk->constants.string_map);
  while (data < strings) {
    u32 key = *((u32*)data);
    data += sizeof(key);
    u32 val = *((u32*)data);
    data += sizeof(val);
    MapSet(&chunk->constants.string_map, key, val);
  }

  u32 strings_size = header.constant_offset - header.string_offset;
  chunk->constants.strings = NewVec(char, strings_size);
  while (data < constants) {
    VecPush(chunk->constants.strings, *data);
    data++;
  }

  u32 num_consts = (header.code_offset - header.constant_offset) / sizeof(Val);
  chunk->constants.values = NewVec(Val, num_consts);
  while (data < code) {
    Val value = *((Val*)data);
    data += sizeof(Val);
    VecPush(chunk->constants.values, value);
  }

  u32 code_size = header.chunk_size - header.code_offset;
  chunk->data = NewVec(u8, code_size);
  while (data < end) {
    VecPush(chunk->data, *data);
    data++;
  }

  return true;
}
