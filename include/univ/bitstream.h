#pragma once

/*
 * Bit stream reader/writer.
 *
 * To read bits from a source, initialize a stream with the data and call ReadBits until HasBits
 * returns false.
 *
 * To write bits to a source, initialize a stream with an allocated pointer, or null. Then call
 * WriteBits as often as needed. Finally, call FinalizeBits to free unused space.
 */

typedef struct {
  u8 *data;
  u32 length;
  u32 capacity;
  u32 index;
  u32 bit;
  enum {lsb, msb} dir;
} BitStream;

/* Initializes a stream with the given data and length in bytes. To allocate a new, writable stream,
 * pass null for data. */
void InitBitStream(BitStream *stream, void *data, u32 length, bool msb);

/* Returns whether the stream index is at the stream length */
bool HasBits(BitStream *stream);

/* Reads n bits from a stream, returning as the low bits of a word. */
u32 ReadBits(BitStream *stream, u32 n);

/* Writes the n lowest bits of a symbol into a stream. */
void WriteBits(BitStream *stream, u32 sym, u32 n);

/* Frees the unused space in an allocated writable stream. This should only be called if the data
 * given to the stream was null or an allocated pointer. */
void *FinalizeBits(BitStream *stream);
