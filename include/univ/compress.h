#pragma once
#include "univ/bitstream.h"

/*
LZW compression (GIF-flavored). Basic byte-oriented compression/decompression
is available with Compress/Decompress. The dst parameter is assigned the
resulting data, and its length is returned.
*/

u32 Compress(void *src, u32 srcLen, u8 **dst);
u32 Decompress(void *src, u32 srcLen, u8 **dst);


/*
Compression can be done in steps. A compressor is created by passing an empty
bitstream to NewCompressor. Then CompressStep should be called repeatedly with
each symbol. A stream must then be finished by calling CompressFinish. The
compressed data is written to the bitstream data, which also has its length.

Decompression can be done in steps. A compressor is created by passing a
bitstream of compressed data to NewCompressor. Then DecompressStep should be
called repeatedly, which returns the next symbol. If the symbol is the stop code
(which is returned by StopCode), decompression is done.

See the source of Compress and Decompress for details.
*/

typedef struct Compressor Compressor;

Compressor *NewCompressor(BitStream *stream, u32 symbolSize);
void FreeCompressor(Compressor *c);

void CompressStep(Compressor *c, u32 symbol);
void CompressFinish(Compressor *c);
u32 DecompressStep(Compressor *c);
u32 StopCode(Compressor *c);
