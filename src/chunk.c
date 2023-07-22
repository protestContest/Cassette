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
  Print("────┬─Chunk──────\n");
  for (u32 i = 0; i < VecCount(chunk->data);) {
    PrintIntN(i, 4, ' ');
    Print("│ ");
    PrintInstruction(chunk, i);
    Print("\n");
    i += OpLength(chunk->data[i]);
  }
  Print("\n");
}

void AppendChunk(Chunk *base, Chunk *chunk)
{
  u32 base_length = VecCount(base->data);

  for (u32 i = 0; i < VecCount(chunk->data);) {
    OpCode op = chunk->data[i];

    PushByte(base, op);
    switch (OpArgType(op)) {
    case ArgsNone:
      break;
    case ArgsLoc: {
      i32 loc = RawInt(ChunkConst(chunk, i+1));
      PushConst(base, IntVal(loc + base_length));
      break;
    }
    case ArgsConst:
      PushConst(base, ChunkConst(chunk, chunk->data[i+1]));
      break;
    case ArgsLocConst: {
      i32 loc = RawInt(ChunkConst(chunk, i+1));
      PushConst(base, IntVal(loc + base_length));
      PushConst(base, ChunkConst(chunk, chunk->data[i+2]));
      break;
    }
    case ArgsReg:
      PushByte(base, chunk->data[i+1]);
      break;
    }

    i += OpLength(op);
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

static u8 chunk_tag[4] = {0xCA, 0x55, 0xE7, 0x7E};
static ChunkFileHeader ChunkSize(Chunk *chunk)
{
  ChunkFileHeader header;
  Copy(chunk_tag, header.id, sizeof(chunk_tag));
  header.version = 1;

  u32 size = sizeof(header);
  u32 num_strings = HashMapCount(&chunk->constants.string_map);
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

bool SniffChunk(char *filename)
{
  return SniffFile(filename, *((u32*)chunk_tag));
}

u8 *SerializeChunk(Chunk *chunk)
{
  ChunkFileHeader header = ChunkSize(chunk);
  u8 *serialized = NewVec(u8, sizeof(header) + header.chunk_size);

  Copy(&header, serialized, sizeof(header));
  u32 cur = sizeof(header);

  u32 num_strings = HashMapCount(&chunk->constants.string_map);
  for (u32 i = 0; i < num_strings; i++) {
    u32 key = GetHashMapKey(&chunk->constants.string_map, i);
    u32 val = GetHashMapValue(&chunk->constants.string_map, i);

    *((u32*)(serialized+cur)) = key;
    cur += sizeof(key);
    *((u32*)(serialized+cur)) = val;
    cur += sizeof(val);
  }

  Assert(cur == header.string_offset);
  Copy(chunk->constants.strings, serialized+cur, VecCount(chunk->constants.strings));
  cur += VecCount(chunk->constants.strings);
  Assert(cur == header.constant_offset);
  Copy(chunk->constants.values, serialized+cur, VecCount(chunk->constants.values)*sizeof(Val));
  cur += VecCount(chunk->constants.values)*sizeof(Val);
  Assert(cur == header.code_offset);
  Copy(chunk->data, serialized+cur, VecCount(chunk->data));
  cur += VecCount(chunk->data);
  Assert(cur == header.chunk_size);

  RawVecCount(serialized) = sizeof(header) + header.chunk_size;
  return serialized;
}

bool WriteChunk(Chunk *chunk, char *filename)
{
  u8 *serialized = SerializeChunk(chunk);
  bool result = WriteFile(filename, serialized, VecCount(serialized));
  FreeVec(serialized);
  return result;
}

void DeserializeChunk(u8 *serialized, Chunk *chunk)
{
  ChunkFileHeader header;
  Copy(serialized, &header, sizeof(header));

  u8 *strings = serialized + header.string_offset;
  u8 *constants = serialized + header.constant_offset;
  u8 *code = serialized + header.code_offset;
  u8 *end = serialized + header.chunk_size;

  u32 cur = sizeof(header);
  InitHashMap(&chunk->constants.string_map);
  while (serialized+cur < strings) {
    u32 key = *((u32*)(serialized+cur));
    cur += sizeof(key);
    u32 val = *((u32*)(serialized+cur));
    cur += sizeof(val);
    HashMapSet(&chunk->constants.string_map, key, val);
  }

  u32 strings_len = constants - strings;
  chunk->constants.strings = NewVec(char, strings_len);
  Copy(serialized+cur, chunk->constants.strings, strings_len);
  RawVecCount(chunk->constants.strings) = strings_len;
  cur += strings_len;

  u32 constants_len = code - constants;
  chunk->constants.values = NewVec(Val, constants_len);
  Copy(serialized+cur, chunk->constants.values, constants_len);
  RawVecCount(chunk->constants.values) = constants_len / sizeof(Val);
  cur += constants_len;

  u32 code_len = end - code;
  chunk->data = NewVec(u8, code_len);
  Copy(serialized+cur, chunk->data, code_len);
  RawVecCount(chunk->data) = code_len;
}

bool ReadChunk(Chunk *chunk, char *filename)
{
  u8 *data = ReadFile(filename);
  if (data == NULL) return false;

  DeserializeChunk(data, chunk);
  return true;
}

void EmbedChunk(char *filename, char *name)
{
  u8 *data = ReadFile(filename);
  if (data == NULL) return;

  ChunkFileHeader header;
  Copy(data, &header, sizeof(header));
  u32 size = sizeof(header) + header.chunk_size;

  CEmbed(name, data, size);
}
