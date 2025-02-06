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
#define TableFull(t) (NextCode(t) >= (1 << MAX_CODE_SIZE))

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

struct Compressor {
  BitStream stream;
  u32 symbolSize;
  u32 codeSize;
  u32 clearCode;
  u32 stopCode;
  u32 prefix;
  TableItem *table;
  u32 *outputBuf;
  bool done;
  i32 metric;
};

#define CLEAR_THRESHHOLD (-10)

#define IsSymbolCode(code, symbolSize) ((code) < (1 << (symbolSize)))

Compressor *NewCompressor(void *data, u32 length, u32 symbolSize)
{
  Compressor *c = malloc(sizeof(Compressor));
  InitBitStream(&c->stream, data, length, false);
  c->symbolSize = symbolSize;
  c->table = CreateTable(symbolSize);
  c->clearCode = TableAdd(&c->table, NullCode, NullCode);
  c->stopCode = TableAdd(&c->table, NullCode, NullCode);
  c->codeSize = symbolSize + 1;
  c->prefix = NullCode;
  c->outputBuf = 0;
  c->done = false;
  c->metric = 0;
  return c;
}

void FreeCompressor(Compressor *c)
{
  FreeTable(c->table);
  FreeVec(c->outputBuf);
  free(c);
}

u32 StopCode(Compressor *c)
{
  return c->stopCode;
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
  c->metric = 0;
}

void CompressStep(Compressor *c, u32 sym)
{
  u32 code;

  if (c->done) return;

  code = TableFind(c->table, c->prefix, sym);

  if (code != NullCode) {
    c->prefix = code;
  } else {
    WriteBits(&c->stream, c->prefix, c->codeSize);

    if (!TableFull(c->table)) {
      TableAdd(&c->table, c->prefix, sym);
      if (NextCode(c->table) == (1 << c->codeSize) &&
          c->codeSize < MAX_CODE_SIZE) {
        c->codeSize++;
      }
    } else {
      /* table full */
      /* record compression metrics (subtract one if symbol code, else add one) */
      c->metric += 2*(-IsSymbolCode(c->prefix, c->symbolSize)) + 1;

      if (c->metric < CLEAR_THRESHHOLD) {
        WriteBits(&c->stream, c->clearCode, c->codeSize);
        ClearTable(c);
      }
    }

    c->prefix = sym;
  }
}

void CompressFinish(Compressor *c)
{
  WriteBits(&c->stream, c->prefix, c->codeSize);
  WriteBits(&c->stream, c->stopCode, c->codeSize);
  FinalizeBits(&c->stream);
  c->done = true;
}

static u32 BufferSymbols(Compressor *c, u32 code)
{
  u32 firstSym;

  do {
    VecPush(c->outputBuf, c->table[code].symbol);
    firstSym = c->table[code].symbol;
    code = c->table[code].prefix;
  } while (code != NullCode);

  return firstSym;
}

u32 DecompressStep(Compressor *c)
{
  u32 code, firstSym, symbol;

  if (VecCount(c->outputBuf) > 0) {
    return VecPop(c->outputBuf);
  }

  if (!HasBits(&c->stream)) c->done = true;
  if (c->done) return 0;

  code = ReadBits(&c->stream, c->codeSize);

  if (code == c->stopCode) {
    c->done = true;
    return 0;
  } else if (code == c->clearCode) {
    ClearTable(c);
    c->prefix = NullCode;
    return DecompressStep(c);
  }

  if (code < NextCode(c->table)) {
    firstSym = BufferSymbols(c, code);

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
    BufferSymbols(c, symbol);
  }

  if (NextCode(c->table) == (1 << c->codeSize) - 1 && c->codeSize < MAX_CODE_SIZE) {
    c->codeSize++;
  }

  c->prefix = code;

  return VecPop(c->outputBuf);
}

u32 Compress(void *src, u32 srcLen, u8 **dst)
{
  Compressor *c;
  u8 *bytes = src;
  u8 *end = bytes + srcLen;
  u32 len;

  c = NewCompressor(0, 0, 8);

  while (bytes < end) {
    u32 sym = *bytes++;
    CompressStep(c, sym);
  }

  CompressFinish(c);
  *dst = c->stream.data;
  len = c->stream.length;
  FreeCompressor(c);

  return len;
}

u32 Decompress(void *src, u32 srcLen, u8 **dst)
{
  Compressor *c;
  u32 sym;
  u32 cap = 0, count = 0;
  u8 *buf = 0;

  c = NewCompressor(src, srcLen, 8);

  while (!c->done) {
    sym = DecompressStep(c);
    if (sym == StopCode(c)) break;

    if (count+1 >= cap) {
      cap = Max(256, 2*cap);
      buf = realloc(buf, cap);
    }
    buf[count++] = (u8)sym;
  }

  FreeCompressor(c);

  *dst = realloc(buf, count);
  return count;
}
