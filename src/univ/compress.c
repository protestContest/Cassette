#include "univ/compress.h"
#include "univ/bitstream.h"
#include "univ/vec.h"

#define MAX_CODE_SIZE 12

typedef struct {
  u32 prefix;
  u32 symbol;
  u32 suffixes;
  u32 left;
  u32 right;
} TableItem;

#define NullCode ((u32)-1)

static void IndexItem(TableItem *table, u32 code, u32 prefix, u32 sym)
{
  u32 index;

  if (prefix == NullCode) return;
  index = table[prefix].suffixes;

  if (index == NullCode) {
    table[prefix].suffixes = code;
    return;
  }

  while (true) {
    if (sym < table[index].left) {
      if (table[index].left == NullCode) {
        table[index].left = code;
        return;
      }
      index = table[index].left;
    } else {
      if (table[index].right == NullCode) {
        table[index].right = code;
        return;
      }
      index = table[index].right;
    }
  }
}

static u32 TableAdd(TableItem **table, u32 prefix, u32 sym)
{
  TableItem item;
  u32 code = VecCount(*table);

  item.prefix = prefix;
  item.symbol = sym;
  item.suffixes = NullCode;
  item.left = NullCode;
  item.right = NullCode;
  VecPush(*table, item);

  IndexItem(*table, code, prefix, sym);

  return code;
}

static TableItem *CreateTable(u32 symbolSize)
{
  u32 i;
  u32 numSymbols = 1 << symbolSize;
  TableItem *table = NewVec(TableItem, numSymbols);
  for (i = 0; i < numSymbols; i++) {
    TableAdd(&table, NullCode, i);
  }
  return table;
}

static void FreeTable(TableItem *table)
{
  FreeVec(table);
}

static u32 TableFind(TableItem *table, u32 prefix, u32 sym)
{
  u32 code;

  if (prefix == NullCode) return sym;

  code = table[prefix].suffixes;
  while (code != NullCode) {
    if (sym == table[code].symbol) return code;

    if (sym < table[code].symbol) {
      code = table[code].left;
    } else {
      code = table[code].right;
    }
  }
  return NullCode;
}

static u32 NextCode(TableItem *table)
{
  return VecCount(table);
}

typedef struct {
  u32 symbolSize;
  u32 codeSize;
  u32 clearCode;
  u32 stopCode;
  u32 prefix;
  TableItem *table;
  u32 *outputBuf;
  bool done;
} Compressor;

void InitCompressor(Compressor *c, u32 symbolSize)
{
  c->symbolSize = symbolSize;
  c->table = CreateTable(symbolSize);
  c->clearCode = TableAdd(&c->table, NullCode, NullCode);
  c->stopCode = TableAdd(&c->table, NullCode, NullCode);
  c->codeSize = symbolSize + 1;
  c->prefix = NullCode;
  c->outputBuf = 0;
  c->done = false;
}

void DestroyCompressor(Compressor *c)
{
  FreeTable(c->table);
  FreeVec(c->outputBuf);
}

static void ClearTable(Compressor *c)
{
  u32 numSymbols = 1 << c->symbolSize;
  u32 i;
  VecTrunc(c->table, numSymbols + 2);
  for (i = 0; i < numSymbols; i++) {
    c->table[i].suffixes = NullCode;
    c->table[i].left = NullCode;
    c->table[i].right = NullCode;
  }
  c->codeSize = c->symbolSize + 1;
}

void CompressStep(Compressor *c, BitStream *input, BitStream *output)
{
  u32 sym, code;

  if (c->done) return;

  sym = ReadBits(input, c->symbolSize);
  code = TableFind(c->table, c->prefix, sym);

  if (code != NullCode) {
    c->prefix = code;
  } else {
    WriteBits(output, c->prefix, c->codeSize);
    TableAdd(&c->table, c->prefix, sym);
    c->prefix = sym;

    if (NextCode(c->table) == (1 << c->codeSize)) {
      if (c->codeSize == MAX_CODE_SIZE) {
        WriteBits(output, c->clearCode, c->codeSize);
        ClearTable(c);
      } else {
        c->codeSize++;
      }
    }
  }

  if (!HasBits(input)) {
    WriteBits(output, c->prefix, c->codeSize);
    WriteBits(output, c->stopCode, c->codeSize);
    c->done = true;
  }
}

static u32 WriteString(Compressor *c, u32 code, BitStream *output)
{
  u32 firstSym;

  do {
    VecPush(c->outputBuf, c->table[code].symbol);
    firstSym = c->table[code].symbol;
    code = c->table[code].prefix;
  } while (code != NullCode);
  while (VecCount(c->outputBuf) > 0) {
    u32 symbol = VecPop(c->outputBuf);
    WriteBits(output, symbol, c->symbolSize);
  }
  return firstSym;
}

void DecompressStep(Compressor *c, BitStream *input, BitStream *output)
{
  u32 code, firstSym, symbol;

  if (!HasBits(input)) c->done = true;
  if (c->done) return;

  code = ReadBits(input, c->codeSize);

  if (code == c->stopCode) {
    c->done = true;
    return;
  } else if (code == c->clearCode) {
    ClearTable(c);
    c->prefix = NullCode;
    return;
  }

  if (code < NextCode(c->table)) {
    firstSym = WriteString(c, code, output);

    if (c->prefix != NullCode) {
      TableAdd(&c->table, c->prefix, firstSym);
    }
  } else {
    u32 index = c->prefix;
    while (c->table[index].prefix != NullCode) {
      index = c->table[index].prefix;
    }
    firstSym = c->table[index].symbol;
    symbol = TableAdd(&c->table, c->prefix, firstSym);
    WriteString(c, symbol, output);
  }

  if (NextCode(c->table) == (1 << c->codeSize) - 1 && c->codeSize < MAX_CODE_SIZE) {
    c->codeSize++;
  }

  c->prefix = code;
}

u32 Compress(void *src, u32 srcLen, u8 **dst, u32 symbolSize)
{
  BitStream reader;
  BitStream writer;
  Compressor c;

  *dst = 0;
  InitBitStream(&reader, src, srcLen);
  InitBitStream(&writer, *dst, 0);
  InitCompressor(&c, symbolSize);

  while (!c.done) {
    CompressStep(&c, &reader, &writer);
  }

  DestroyCompressor(&c);

  *dst = writer.data;
  return writer.length;
}

u32 Decompress(void *src, u32 srcLen, u8 **dst, u32 symbolSize)
{
  BitStream reader;
  BitStream writer;
  Compressor c;

  *dst = 0;
  InitBitStream(&reader, src, srcLen);
  InitBitStream(&writer, *dst, 0);
  InitCompressor(&c, symbolSize);

  while (!c.done) {
    DecompressStep(&c, &reader, &writer);
  }

  DestroyCompressor(&c);

  *dst = writer.data;
  return writer.length;
}
