/*
A chunk is an executable unit. It holds executable bytecode, an array of
constants (which bytecode can reference), a symbol table storing the names of
all its symbols, a map from byecode indexes to filenames they originate from,
and a map from bytecode indexes to source file indexes.

The file map is an array of (symbol, length) pairs. The symbol's name (in the
chunk's symbol table) is the filename, and the length is how many bytes of code
originate from the file. The file map entries are stored sequentially, so the
entry for a given bytecode index can be found by scanning the map, incrementing
a cursor with each entry's length until the index falls within the entry. If a
chunk starts with bytes not derived from a file, an initial entry with filename
"*init*" is added to the beginning.

The source map is similar to the file map, except it stores (delta, length)
pairs as bytes. A cursor must be used to track the source file position, which
initially starts at 0. Each delta is a signed byte that moves the cursor to a
new source position, and its associated length (an unsigned byte) is how many
bytes of code originate from the cursor position. If a source map entry's delta
can't fit in a signed byte, multiple entries of length 0 are added to move the
cursor. If an entry's length can't fit in an unsigned byte, multiple entries
with delta 0 are added. The special delta value -128 resets the cursor to 0.
This happens at file boundaries in the bytecode. The source position for a given
bytecode index can be found by scanning the map, adjusting a source position
cursor and incrementing a bytecode position cursor, until the index falls within
the source map entry.
*/

#include "chunk.h"
#include "ops.h"
#include "version.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"

#define SourceMapReset  -128

static u32 AddConst(Val value, Chunk *chunk);

void InitChunk(Chunk *chunk)
{
  InitVec((Vec*)&chunk->code, sizeof(u8), 256);
  InitVec((Vec*)&chunk->constants, sizeof(Val), 256);
  InitSymbolTable(&chunk->symbols);
  InitVec((Vec*)&chunk->source_map, sizeof(u8), 256);
  InitVec((Vec*)&chunk->file_map, sizeof(u32), 8);
}

void DestroyChunk(Chunk *chunk)
{
  DestroyVec((Vec*)&chunk->code);
  DestroyVec((Vec*)&chunk->constants);
  DestroySymbolTable(&chunk->symbols);
  DestroyVec((Vec*)&chunk->source_map);
  DestroyVec((Vec*)&chunk->file_map);
}

/* Pushes a byte into a chunk's code. Also updates the source map and file map. */
u32 PushByte(u8 byte, Chunk *chunk)
{
  u32 pos = chunk->code.count;

  ByteVecPush(&chunk->code, byte);

  Assert(chunk->source_map.count > 0);
  VecPeek(&chunk->source_map, 0)++;
  if (VecPeek(&chunk->source_map, 0) == 255) {
    /* If the current source map entry is bigger than a byte can hold, make a new entry */
    PushSourcePos(0, chunk);
  }

  Assert(chunk->file_map.count > 0);
  VecPeek(&chunk->file_map, 0)++;
  if (VecPeek(&chunk->file_map, 0) == MaxUInt) {

  }

  return pos;
}

/* Returns the index of a constant from a chunk's const array, or -1 if not found */
static i32 FindConst(Val value, Chunk *chunk)
{
  i32 i;
  for (i = 0; i < (i32)chunk->constants.count; i++) {
    if (value == VecRef(&chunk->constants, i)) {
      return i;
    }
  }
  return -1;
}

/* Pushes a const into a chunk, returns its index */
static u32 AddConst(Val value, Chunk *chunk)
{
  IntVecPush(&chunk->constants, value);
  return chunk->constants.count - 1;
}

/* Pushes a constant op code into a chunk. If the constant is an integer < 256,
   it pushes OpInt instead. If the constant is in the const array, it uses its
   index, otherwise it adds it. If the const array has more than 256 items, this
   pushes OpConst2. */
u32 PushConst(Val value, Chunk *chunk)
{
  u32 pos = chunk->code.count;
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) <= 255) {
    u8 byte = RawInt(value);
    PushByte(OpInt, chunk);
    PushByte(byte, chunk);
  } else if (value == Nil) {
    PushByte(OpNil, chunk);
  } else {
    i32 index = FindConst(value, chunk);
    if (index < 0) {
      index = AddConst(value, chunk);
    }

    if (index < 256) {
      PushByte(OpConst, chunk);
      PushByte((u8)index, chunk);
    } else {
      PushByte(OpConst2, chunk);
      PushByte((u8)((index >> 8) & 0xFF), chunk);
      PushByte((u8)(index & 0xFF), chunk);
    }
  }
  return pos;
}

void PatchConst(Chunk *chunk, u32 index)
{
  u32 dist = chunk->code.count - index - 3;
  if (dist < 256) {
    chunk->code.items[index] = OpInt;
    chunk->code.items[index+1] = (u8)dist;
  } else if (chunk->constants.count < 256) {
    chunk->code.items[index] = OpConst;
    chunk->code.items[index+1] = AddConst(IntVal(dist), chunk);
  } else {
    u32 const_idx = AddConst(IntVal(dist), chunk);
    chunk->code.items[index] = OpConst2;
    chunk->code.items[index+1] = (u8)((const_idx >> 8) & 0xFF);
    chunk->code.items[index+2] = (u8)(const_idx & 0xFF);
  }
}

/* The file map is an array of (symbol, length) pairs, where the symbol's name is
   a filename (in the chunk's symbol table), and the length is how many bytes of
   code in the chunk map to the file. This lets us find the file for a specific
   position in the code. */
void PushFilePos(Val filename, Chunk *chunk)
{
  IntVecPush(&chunk->file_map, filename);
  IntVecPush(&chunk->file_map, 0);

  /* reset the source map counter with a new source map position */
  ByteVecPush(&chunk->source_map, SourceMapReset);
  ByteVecPush(&chunk->source_map, 0);
}

/* Adds a new entry to the source map. If the previous entry's length was 0, it
   is replaced. */
void PushSourcePos(i32 pos, Chunk *chunk)
{
  u8 last_length;
  i8 last_pos;

  if (chunk->source_map.count > 0) {
    if (pos == 0) return;
    last_length = VecPeek(&chunk->source_map, 0);
    last_pos = (i8)VecPeek(&chunk->source_map, 1);
    if (last_length == 0 && last_pos != SourceMapReset) {
      pos += (i8)VecPeek(&chunk->source_map, 1);
      chunk->source_map.count -= 2;
    }
  }

  while (pos > 127) {
    ByteVecPush(&chunk->source_map, 127);
    ByteVecPush(&chunk->source_map, 0);
    pos -= 127;
  }
  while (pos < -127) {
    ByteVecPush(&chunk->source_map, -127);
    ByteVecPush(&chunk->source_map, 0);
    pos += 127;
  }
  ByteVecPush(&chunk->source_map, (i8)pos);
  ByteVecPush(&chunk->source_map, 0);
}

/* Returns the filename for a byte position in the chunk */
char *ChunkFileAt(u32 pos, Chunk *chunk)
{
  u32 size = 0;
  u32 i;

  /* step through each file map entry until the byte position falls with in one */
  for (i = 0; i < chunk->file_map.count; i += 2) {
    if (size + chunk->file_map.items[i+1] > pos) {
      return SymbolName(chunk->file_map.items[i], &chunk->symbols);
    }
    size += chunk->file_map.items[i+1];
  }

  return 0;
}

/* Returns how many bytes of code the file at the given position occupies */
u32 ChunkFileByteSize(u32 pos, Chunk *chunk)
{
  u32 size = 0;
  u32 i;

  for (i = 0; i < chunk->file_map.count; i += 2) {
    if (size + chunk->file_map.items[i+1] > pos) {
      return chunk->file_map.items[i+1];
    }
    size += chunk->file_map.items[i+1];
  }

  return 0;
}

/* Finds the position in a source file that a bytecode index maps to. */
u32 SourcePosAt(u32 index, Chunk *chunk)
{
  ByteVec map = chunk->source_map;
  u32 i, source_pos = 0, code_pos = 0;

  for (i = 0; i < map.count; i += 2) {
    if ((i8)map.items[i] == SourceMapReset) {
      source_pos = 0;
    } else {
      source_pos += (i8)map.items[i];
    }

    if (code_pos + map.items[i+1] > index) {
      return source_pos;
    }

    code_pos += map.items[i+1];
  }

  return -1;
}

/* Chunk file format: (big-endian)
4 byte file tag: CA 55 E7 7E
u16 file version number
u16 runtime version number
u32 length of bytecode
u32 number of constants
u32 number of file map entries
u32 number of source map entries
u32 length of symbol names (in bytes)
bytecode (size = length of bytecode, aligned to 4 bytes)
constants (size = num constants * sizeof(Val))
file map entries (size = num entries * sizeof(u32) * 2)
source map entries (size = num entries * 2, aligned to 4 bytes)
symbol names (null-terminated, size = symbol name length, aligned to 4 bytes)
u32 checksum
*/

static u8 chunk_tag[4] = {0xCA, 0x55, 0xE7, 0x7E};
#define FormatVersion 1
#define ChunkHeaderSize (sizeof(chunk_tag) + 2*sizeof(u16) + 5*sizeof(u32))
#define ChunkTrailerSize sizeof(u32)

static u32 WritePadding(u8 *data, u32 size)
{
  u32 i;
  u32 n = Align(size, 4) - size;
  for (i = 0; i < n; i++) *data++ = 0;
  return n;
}

ByteVec SerializeChunk(Chunk *chunk)
{
  ByteVec serialized;
  u8 *cur;
  u32 i, size;

  size = ChunkHeaderSize
    + Align(chunk->code.count, 4)
    + sizeof(Val)*chunk->constants.count
    + sizeof(u32)*chunk->file_map.count
    + Align(chunk->source_map.count, 4)
    + Align(chunk->symbols.names.count, 4)
    + ChunkTrailerSize;

  serialized.capacity = size;
  serialized.count = size;
  serialized.items = Alloc(size);
  cur = serialized.items;

  Copy(chunk_tag, cur, 4);
  cur += 4;
  WriteShort(FormatVersion, cur);
  cur += sizeof(u16);
  WriteShort(VERSION, cur);
  cur += sizeof(u16);
  WriteInt(chunk->code.count, cur);
  cur += sizeof(u32);
  WriteInt(chunk->constants.count, cur);
  cur += sizeof(u32);
  WriteInt(chunk->file_map.count / 2, cur);
  cur += sizeof(u32);
  WriteInt(chunk->source_map.count / 2, cur);
  cur += sizeof(u32);
  WriteInt(chunk->symbols.names.count, cur);
  cur += sizeof(u32);

  Copy(chunk->code.items, cur, chunk->code.count);
  cur += chunk->code.count;
  cur += WritePadding(cur, chunk->code.count);

  for (i = 0; i < chunk->constants.count; i++) {
    WriteInt(VecRef(&chunk->constants, i), cur);
    cur += sizeof(Val);
  }

  for (i = 0; i < chunk->file_map.count; i++) {
    WriteInt(chunk->file_map.items[i], cur);
    cur += sizeof(u32);
  }

  Copy(chunk->source_map.items, cur, chunk->source_map.count);
  cur += chunk->source_map.count;
  cur += WritePadding(cur, chunk->source_map.count);

  Copy(chunk->symbols.names.items, cur, chunk->symbols.names.count);
  cur += chunk->symbols.names.count;
  cur += WritePadding(cur, chunk->symbols.names.count);

  /* checksum */
  WriteInt(0, cur);

  return serialized;
}

Result DeserializeChunk(u8 *start, u32 size)
{
  Chunk *chunk;
  u32 symbols_size, i, size_read = 0, checksum = 0;
  u8 *data = start;

  if (size < ChunkHeaderSize) return ErrorResult("Bad file: too small", 0, 0);
  if (!MemEq(data, chunk_tag, 4)) return ErrorResult("Bad file: tag mismatch", 0, 0);
  data += 4;
  size_read = ChunkHeaderSize + ChunkTrailerSize;

  if (ReadShort(data) != FormatVersion) return ErrorResult("Unsupported format", 0, 0);
  data += sizeof(u16);
  if (ReadShort(data) != VERSION) return ErrorResult("Unsupported runtime", 0, 0);
  data += sizeof(u16);

  chunk = Alloc(sizeof(Chunk));
  chunk->code.count = ReadInt(data);
  chunk->code.capacity = chunk->code.count;
  data += sizeof(u32);
  size_read += Align(chunk->code.count, 4);

  chunk->constants.count = ReadInt(data);
  chunk->constants.capacity = chunk->constants.count;
  data += sizeof(u32);
  size_read += sizeof(Val)*chunk->constants.count;

  chunk->file_map.count = ReadInt(data) * 2;
  chunk->file_map.capacity = chunk->file_map.count;
  data += sizeof(u32);
  size_read += sizeof(u32)*chunk->file_map.count;

  chunk->source_map.count = ReadInt(data) * 2;
  chunk->source_map.capacity = chunk->source_map.count;
  data += sizeof(u32);
  size_read += Align(chunk->source_map.count, 4);

  symbols_size = ReadInt(data);
  data += sizeof(u32);
  size_read += Align(symbols_size, 4);

  if (size_read != size) {
    Free(chunk);
    return ErrorResult("Bad file: size mismatch", 0, 0);
  }

  chunk->code.items = Alloc(chunk->code.count);
  chunk->constants.items = Alloc(sizeof(Val)*chunk->constants.count);
  chunk->file_map.items = Alloc(sizeof(u32)*chunk->file_map.count);
  chunk->source_map.items = Alloc(chunk->source_map.count);

  Copy(data, chunk->code.items, chunk->code.count);
  data += chunk->code.count;
  data += Align(chunk->code.count, 4) - chunk->code.count;

  for (i = 0; i < chunk->constants.count; i++) {
    chunk->constants.items[i] = ReadInt(data);
    data += sizeof(Val);
  }

  for (i = 0; i < chunk->file_map.count; i++) {
    chunk->file_map.items[i] = ReadInt(data);
    data += sizeof(u32);
  }

  Copy(data, chunk->source_map.items, chunk->source_map.count);
  data += chunk->source_map.count;
  data += Align(chunk->source_map.count, 4) - chunk->source_map.count;

  DeserializeSymbols(symbols_size, (char*)data, &chunk->symbols);
  data += symbols_size;
  data += Align(symbols_size, 4) - symbols_size;

  if (ReadInt(data) != checksum) {
    DestroyChunk(chunk);
    return ErrorResult("Invalid checksum", 0, 0);
  }

  return ItemResult(chunk);
}

bool IsBinaryChunk(char *filename)
{
  u8 buf[sizeof(chunk_tag)];
  int file = Open(filename);
  if (file < 0) return false;
  Read(file, buf, sizeof(chunk_tag));
  Close(file);
  return MemEq(buf, chunk_tag, sizeof(chunk_tag));
}
