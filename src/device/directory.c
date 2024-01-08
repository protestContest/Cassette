#include "directory.h"
#include "mem/symbols.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define SymDevice         0x7FD945B8 /* device */
#define SymDirectory      0x7FD7808F /* directory */
#define SymPipe           0x7FDB4D2F /* pipe */
#define SymLink           0x7FD10FD6 /* link */
#define SymFile           0x7FDB7595 /* file */
#define SymSocket         0x7FDAE29C /* socket */
#define SymUnknown        0x7FDD6BDA /* unknown */
#define SymPath           0x7FDE8EB6 /* path */

typedef struct {
  DIR *dir;
  char *path;
} DirContext;

Result DirectoryOpen(Val opts, Mem *mem)
{
  char *path;
  char *resolved_path;
  DIR *dir;
  DirContext *ctx;

  if (TupleCount(opts, mem) != 1) return ErrorResult("Expected {path}", 0, 0);
  if (!IsBinary(TupleGet(opts, 0, mem), mem)) return ErrorResult("Expected {path}", 0, 0);
  path = CopyStr(BinaryData(TupleGet(opts, 0, mem), mem), BinaryCount(TupleGet(opts, 0, mem), mem));

  if (!DirExists(path)) {
    Free(path);
    return ErrorResult("Directory does not exist", 0, 0);
  }

  dir = opendir(path);
  resolved_path = realpath(path, 0);
  Free(path);
  if (!resolved_path) {
    return ErrorResult(strerror(errno), 0, 0);
  }

  ctx = Alloc(sizeof(DirContext));
  ctx->dir = dir;
  ctx->path = resolved_path;
  return ItemResult(ctx);
}

Result DirectoryClose(void *context, Mem *mem)
{
  DirContext *ctx = (DirContext*)context;
  closedir(ctx->dir);
  Free(ctx->path);
  return ValueResult(Ok);
}

Result DirectoryRead(void *context, Val length, Mem *mem)
{
  DirContext *ctx = (DirContext*)context;
  u32 max = MaxUInt;
  u32 i = 0;
  struct dirent *ent;
  Val items = Nil;

  if (IsInt(length)) {
    max = RawInt(length);
  }

  while (i < max && (ent = readdir(ctx->dir)) != NULL) {
    Val item = MakeTuple(2, mem);
    TupleSet(item, 0, BinaryFrom(ent->d_name, StrLen(ent->d_name), mem), mem);
    switch (ent->d_type) {
    case DT_BLK:
    case DT_CHR:
      TupleSet(item, 1, SymDevice, mem);
      break;
    case DT_DIR:
      TupleSet(item, 1, SymDirectory, mem);
      break;
    case DT_FIFO:
      TupleSet(item, 1, SymPipe, mem);
      break;
    case DT_LNK:
      TupleSet(item, 1, SymLink, mem);
      break;
    case DT_REG:
      TupleSet(item, 1, SymFile, mem);
      break;
    case DT_SOCK:
      TupleSet(item, 1, SymSocket, mem);
      break;
    default:
      TupleSet(item, 1, SymUnknown, mem);
    }

    items = Pair(item, items, mem);
  }
  items = ReverseList(items, Nil, mem);
  return ValueResult(items);
}

Result DirectoryWrite(void *context, Val data, Mem *mem)
{
  DirContext *ctx = (DirContext*)context;
  Val type;
  char *name;
  char *path;
  if (!IsTuple(data, mem)) return ErrorResult("Expected {type, name}", 0, 0);
  if (TupleCount(data, mem) != 2) return ErrorResult("Expected {type, name}", 0, 0);
  type = TupleGet(data, 0, mem);
  if (!IsSym(type)) return ErrorResult("Expected {type, name}", 0, 0);
  if (!IsBinary(TupleGet(data, 1, mem), mem)) return ErrorResult("Expected {type, name}", 0, 0);
  name = CopyStr(BinaryData(TupleGet(data, 1, mem), mem), BinaryCount(TupleGet(data, 1, mem), mem));
  path = JoinStr(ctx->path, name, '/');
  Free(name);

  if (type == SymFile) {
    mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR; /* unix permission 0644 */
    int file = open(path, O_CREAT|O_WRONLY|O_EXCL, mode);
    Free(path);
    if (file == -1) return ErrorResult(strerror(errno), 0, 0);
    close(file);
    return ValueResult(Ok);
  } else if (type == SymDirectory) {
    mode_t mode = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR; /* unix permission 0644 */
    int file = mkdir(path, mode);
    Free(path);
    if (file == -1) return ErrorResult(strerror(errno), 0, 0);
    return ValueResult(Ok);
  } else {
    return ErrorResult("Expected type to be :file or :directory", 0, 0);
  }
}

Result DirectoryGet(void *context, Val key, Mem *mem)
{
  DirContext *ctx = (DirContext*)context;

  if (key == SymPath) {
    Val path = BinaryFrom(ctx->path, StrLen(ctx->path), mem);
    return ValueResult(path);
  } else {
    return ValueResult(Nil);
  }
}
