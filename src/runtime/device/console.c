#include "console.h"
#include "univ/str.h"
#include "univ/system.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

Result ConsoleRead(void *context, Val length, Mem *mem)
{
  char *buf;
  if (!IsInt(length)) return ErrorResult("Expected integer", 0, 0);

  buf = Alloc(length);

  if (!fgets(buf, RawInt(length), stdin)) {
    Free(buf);
    return ErrorResult(strerror(errno), 0, 0);
  } else {
    Val bin;
    u32 length = StrLen(buf);
    if (IsNewline(buf[length-1])) length--;
    bin = BinaryFrom(buf, length, mem);
    return ValueResult(bin);
  }
}

Result ConsoleWrite(void *context, Val data, Mem *mem)
{
  if (IsInt(data) && RawInt(data) >= 0 && RawInt(data) < 256) {
    printf("%c", RawInt(data));
    return ValueResult(IntVal(1));
  } else if (IsBinary(data, mem)) {
    u32 count = BinaryCount(data, mem);
    u32 length = printf("%*.*s", count, count, BinaryData(data, mem));
    fflush(stdout);
    return ValueResult(IntVal(length));
  } else {
    return ErrorResult("Invalid console data", 0, 0);
  }
}
