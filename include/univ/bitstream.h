#pragma once

/*
Bit stream reader/writer. Bits are read/written least-significant-bit first. To
use WriteBits, the data buffer must be an allocated pointer (or 0).
*/

typedef struct {
  u8 *data;
  u32 length;
  u32 capacity;
  u32 index;
  u32 bit;
} BitStream;

void InitBitStream(BitStream *stream, u8 *data, u32 length);
bool HasBits(BitStream *stream);
u32 ReadBits(BitStream *stream, u32 nBits);
void WriteBits(BitStream *stream, u32 sym, u32 nBits);
