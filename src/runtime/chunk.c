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
#include "univ/system.h"
#include "univ/str.h"
#include "ops.h"
#include "debug.h"
#include "univ/math.h"
#include "version.h"

#define SourceMapReset  -128

static u32 AddConst(Val value, Chunk *chunk);

void InitChunk(Chunk *chunk)
{
  InitVec((Vec*)&chunk->code, sizeof(u8), 256);
  InitVec((Vec*)&chunk->constants, sizeof(Val), 256);
  InitSymbolTable(&chunk->symbols);
  DefinePrimitiveSymbols(&chunk->symbols);
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
