#include "console.h"
#include "univ/system.h"
#include "univ/str.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
    return OkResult(bin);
  }
}

Result ConsoleWrite(void *context, Val data, Mem *mem)
{
  if (IsInt(data) && RawInt(data) >= 0 && RawInt(data) < 256) {
    printf("%c", RawInt(data));
    return OkResult(1);
  } else if (IsBinary(data, mem)) {
    u32 length = printf("%*.*s", BinaryLength(data, mem), BinaryLength(data, mem), BinaryData(data, mem));
    fflush(stdout);
    return OkResult(IntVal(length));
  } else {
    return ErrorResult("Invalid console data", 0, 0);
  }
}
