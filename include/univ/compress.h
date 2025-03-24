#pragma once

/*
 * LZW compression (GIF-flavored). For one-shot compression or decompression, use Compress and
 * Decompress.
 *
 * To compress in multiple steps, create a compressor with NewCompressor and call CompressStep with
 * each symbol. Then call CompressFinish and FreeCompressor.
 *
 * To decompress in multiple steps, create a compressor with NewCompressor and call DecompressStep
 * and save each decompressed symbol. When the stop symbol is returned, call FreeCompressor.
 */

/* Compresses src, returning the compressed length and the compressed data in dst, which points to a
 * newly-allocated buffer */
u32 Compress(void *src, u32 srcLen, u8 **dst);

/* Decompresses src, returning the decompressed length and the decompressed data in dst, which
 * points to a newly-allocated buffer */
u32 Decompress(void *src, u32 srcLen, u8 **dst);


/*
 * Compression can be done in steps. A compressor is created by passing an empty bitstream to
 * NewCompressor. Then CompressStep should be called repeatedly with each symbol. A stream must then
 * be finished by calling CompressFinish. The compressed data is written to the bitstream data,
 * which also has its length.
 *
 * Decompression can be done in steps. A compressor is created by passing a bitstream of compressed
 * data to NewCompressor. Then DecompressStep should be called repeatedly, which returns the next
 * symbol. If the symbol is the stop code (which is returned by StopCode), decompression is done.
 *
 * See the source of Compress and Decompress for details.
 */

typedef struct Compressor Compressor;

/* Creates a compressor to compress/decompress data with a given symbol size */
Compressor *NewCompressor(void *data, u32 length, u32 symbolSize);

/* Frees a compressor */
void FreeCompressor(Compressor *c);

/* Adds one symbol to a compression stream */
void CompressStep(Compressor *c, u32 symbol);

/* Finalizes a compression stream */
void CompressFinish(Compressor *c);

/* Returns the next symbol from a decompression stream */
u32 DecompressStep(Compressor *c);

/* Returns the stop symbol for a compression/decompression stream */
u32 StopCode(Compressor *c);

/* Simple Macintosh-style RLE encoding */
void PackBits(void *src, i8 **dst, u32 len);
void UnpackBits(i8 **src, u8 *dst, u32 len);
