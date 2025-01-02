#include "univ/bitstream.h"
#include "univ/math.h"

void InitBitStream(BitStream *stream, u8 *data, u32 length)
{
  stream->data = data;
  stream->length = length;
  stream->capacity = length;
  stream->index = 0;
  stream->bit = 0;
}

bool HasBits(BitStream *stream)
{
  return stream->index < stream->length;
}

u32 ReadBits(BitStream *stream, u32 nBits)
{
  u32 symBit = 0;
  u32 mask;
  u32 sym = 0;
  u8 byteBits;
  u8 bitsRead;

  while (nBits) {
    mask = (1 << nBits) - 1;
    byteBits = (stream->data[stream->index] >> stream->bit) & mask;
    sym |= (u32)byteBits << symBit;
    bitsRead = Min(nBits, 8 - stream->bit);
    stream->bit += bitsRead;
    if (stream->bit == 8) {
      stream->index++;
      stream->bit = 0;
    }
    nBits -= bitsRead;
    symBit += bitsRead;
  }

  return sym;
}

void GrowStream(BitStream *stream, u32 nBits)
{
  u32 bytesFree = stream->capacity - stream->length;
  u32 bitsFree = (bytesFree == 0) ? 0 : bytesFree*8 + ((stream->bit > 0) ? (8 - stream->bit) : 0);
  u32 bitsNeeded, bytesNeeded;
  u32 capacity, i;
  if (bitsFree >= nBits) return;
  bitsNeeded = nBits - bitsFree;
  bytesNeeded = Align(bitsNeeded, 8) / 8;

  capacity = Max(stream->index + bytesNeeded, 2*stream->capacity);
  stream->data = realloc(stream->data, capacity);
  for (i = stream->capacity; i < capacity; i++) stream->data[i] = 0;
  stream->capacity = capacity;
}

void WriteBits(BitStream *stream, u32 sym, u32 nBits)
{
  u32 symBit = 0;
  u32 mask;
  u8 byteBits;
  u8 bitsWritten;

  GrowStream(stream, nBits);

  while (nBits) {
    assert(stream->index < stream->capacity);
    bitsWritten = Min(nBits, 8 - stream->bit);
    mask = (1 << bitsWritten) - 1;
    byteBits = (sym >> symBit) & mask;
    stream->data[stream->index] |= byteBits << stream->bit;
    symBit += bitsWritten;
    stream->bit += bitsWritten;
    if (stream->bit == 8) {
      stream->bit = 0;
      stream->index++;
    }
    nBits -= bitsWritten;
  }

  stream->length = Max(stream->length, stream->index + (stream->bit > 0));
}
