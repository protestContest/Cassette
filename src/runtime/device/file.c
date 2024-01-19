#include "file.h"
#include "runtime/mem/symbols.h"
#include "univ/str.h"
#include "univ/system.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define SymPosition       0x7FDEFA47 /* position */

typedef struct {
  FILE *file;
  char *path;
  char *mode;
} FileContext;

Result FileOpen(Val opts, Mem *mem)
{
  char *path;
  char *mode;
  FILE *file;
  FileContext *context;

  if (TupleCount(opts, mem) != 2) return ErrorResult("Expected {filename, mode}", 0, 0);
  if (!IsBinary(TupleGet(opts, 0, mem), mem)) return ErrorResult("Expected {filename, mode}", 0, 0);
  if (!IsBinary(TupleGet(opts, 1, mem), mem)) return ErrorResult("Expected {filename, mode}", 0, 0);

  path = CopyStr(BinaryData(TupleGet(opts, 0, mem), mem), BinaryCount(TupleGet(opts, 0, mem), mem));
  mode = CopyStr(BinaryData(TupleGet(opts, 1, mem), mem), BinaryCount(TupleGet(opts, 1, mem), mem));
  file = fopen(path, mode);
  if (!file) return ErrorResult(strerror(errno), 0, 0);

  context = (FileContext*)Alloc(sizeof(FileContext));
  context->file = file;
  context->path = path;
  context->mode = mode;

  return ItemResult(context);
}

Result FileClose(void *context, Mem *mem)
{
  FileContext *ctx = (FileContext*)context;
  fclose(ctx->file);
  Free(ctx->path);
  Free(ctx->mode);
  return ValueResult(Ok);
}

Result FileRead(void *context, Val length, Mem *mem)
{
  FileContext *ctx = (FileContext*)context;
  if (!IsInt(length)) return ErrorResult("Expected integer", 0, 0);

  if (RawInt(length) == 0) {
    return ValueResult(MakeBinary(0, mem));
  } else {
    char *buf = Alloc(RawInt(length));
    u32 bytes_read = fread(buf, 1, RawInt(length), ctx->file);
    if (bytes_read == 0) {
      Free(buf);
      if (feof(ctx->file)) {
        return ValueResult(Nil);
      } else if (ferror(ctx->file)) {
        return ErrorResult(strerror(errno), 0, 0);
      } else {
        return ValueResult(MakeBinary(0, mem));
      }
    } else {
      Val bin = BinaryFrom(buf, bytes_read, mem);
      Free(buf);
      return ValueResult(bin);
    }
  }
}

Result FileWrite(void *context, Val data, Mem *mem)
{
  FileContext *ctx = (FileContext*)context;
  if (!IsBinary(data, mem)) return ErrorResult("Expected binary", 0, 0);

  if (BinaryCount(data, mem) == 0) {
    return ValueResult(IntVal(0));
  } else {
    u32 length = BinaryCount(data, mem);
    char *bytes = BinaryData(data, mem);
    u32 bytes_written = fwrite(bytes, 1, length, ctx->file);
    if (bytes_written == length) {
      return ValueResult(Ok);
    } else if (ferror(ctx->file)) {
      return ErrorResult(strerror(errno), 0, 0);
    } else {
      return ValueResult(Nil);
    }
  }
}

Result FileSet(void *context, Val key, Val value, Mem *mem)
{
  FileContext *ctx = (FileContext*)context;
  if (key == SymPosition) {
    if (!IsInt(value)) return ErrorResult("Expected integer", 0, 0);
    fseek(ctx->file, RawInt(value), SEEK_SET);
    return ValueResult(Ok);
  } else {
    return ValueResult(Nil);
  }
}

Result FileGet(void *context, Val key, Mem *mem)
{
  FileContext *ctx = (FileContext*)context;
  if (key == SymPosition) {
    u32 pos = ftell(ctx->file);
    return ValueResult(IntVal(pos));
  } else {
    return ErrorResult("Read only", 0, 0);
  }
}
