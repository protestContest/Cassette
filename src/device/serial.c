#include "serial.h"
#include "univ/system.h"
#include "univ/math.h"
#include "univ/serial.h"
#include "univ/str.h"
#include "mem/symbols.h"
#include <errno.h>
#include <string.h>
#include <unistd.h>

Result SerialOpen(Val opts, Mem *mem)
{
  Val path;
  SerialPort *port;

  if (TupleCount(opts, mem) != 2) return ErrorResult("Expected {path, speed}", 0, 0);
  path = TupleGet(opts, 0, mem);
  if (!IsBinary(path, mem)) return ErrorResult("Expected path to be a string", 0, 0);
  if (!IsInt(TupleGet(opts, 1, mem))) return ErrorResult("Expected speed to be an integer", 0, 0);

  port = Alloc(sizeof(SerialPort));
  port->path = CopyStr(BinaryData(path, mem), BinaryCount(path, mem));

  port->file = OpenSerial(port->path, RawInt(TupleGet(opts, 1, mem)));

  if (port->file == -1) {
    Free(port->path);
    Free(port);
    return ErrorResult(strerror(errno), 0, 0);
  }

  return DataResult(port);
}

Result SerialClose(void *context, Mem *mem)
{
  SerialPort *port = (SerialPort*)context;
  CloseSerial(port->file);
  Free(port->path);
  Free(port);
  return OkResult(Ok);
}

Result SerialRead(void *context, Val length, Mem *mem)
{
  SerialPort *port = (SerialPort*)context;
  if (!IsInt(length)) return ErrorResult("Expected integer", 0, 0);

  if (RawInt(length) == 0) {
    return OkResult(MakeBinary(0, mem));
  } else {
    u32 size = RawInt(length);
    char *buf = Alloc(size);
    char *cur = buf;
    i32 bytes_read = read(port->file, buf, size);
    while (bytes_read > 0) {
      cur += bytes_read;
      bytes_read = read(port->file, cur, size - (cur - buf));
    }
    if (bytes_read < 0) {
      Free(buf);
      return ErrorResult(strerror(errno), 0, 0);
    } else {
      Val bin = BinaryFrom(buf, cur - buf, mem);
      Free(buf);
      return OkResult(bin);
    }
  }
}

Result SerialWrite(void *context, Val data, Mem *mem)
{
  SerialPort *port = (SerialPort*)context;
  if (!IsBinary(data, mem)) return ErrorResult("Expected binary", 0, 0);

  if (BinaryCount(data, mem) == 0) {
    return OkResult(IntVal(0));
  } else {

    u32 size = BinaryCount(data, mem);
    char *buf = BinaryData(data, mem);
    char *cur = buf;
    i32 bytes_written = write(port->file, buf, size);
    while (bytes_written > 0) {
      cur += bytes_written;
      bytes_written = write(port->file, cur, size - (cur - buf));
    }

    if (bytes_written < 0) {
      Free(buf);
      return ErrorResult(strerror(errno), 0, 0);
    } else {
      return OkResult(IntVal(bytes_written));
    }
  }
}

Result SerialSet(void *context, Val key, Val value, Mem *mem)
{
  return ErrorResult("Unimplemented", 0, 0);
}

Result SerialGet(void *context, Val key, Mem *mem)
{
  return ErrorResult("Unimplemented", 0, 0);
}

