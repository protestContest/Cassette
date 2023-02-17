#include "buf.h"
#include "platform/io.h"

void Append(Buf *buf, u8 *src, u32 len)
{
  u8 *end = src + len;
  while (!buf->error && src < end) {
    u32 left = end - src;
    u32 available = buf->capacity - buf->count;
    u32 amount = (available < len) ? available : left;

    for (u32 i = 0; i < amount; i++) {
      buf->data[buf->count + i] = src[i];
    }
    buf->count += amount;
    src += amount;

    if (amount < left) {
      if (buf->file >= 0) {
        Flush(buf);
      } else {
        buf->error = true;
      }
    }
  }
}

void AppendByte(Buf *buf, u8 byte)
{
  Append(buf, &byte, 1);
}

void AppendInt(Buf *buf, i32 num)
{
  u8 tmp[16];
  u8 *end = tmp + sizeof(tmp);
  u8 *cur = end;

  i32 t = (num > 0) ? -num : num;
  do {
    *--cur = '0' - (t % 10);
  } while (t /= 10);
  if (num < 0) {
    *--cur = '-';
  }
}

void AppendFloat(Buf *buf, float num)
{
  u32 precision = 10000;

  if (num < 0) {
    AppendByte(buf, '-');
    num = -num;
  }

  num += 0.5 / precision;
  u32 integral = num;
  u32 fraction = (num - integral)*precision;
  if (fraction < 0) {
    AppendStr(buf, "inf");
  } else {
    AppendInt(buf, integral);
    AppendByte(buf, '.');
    for (u32 i = precision / 10; i > 1; i /= 10) {
      if (i > fraction) {
        AppendByte(buf, '0');
      }
    }
    AppendInt(buf, fraction);
  }
}

void Flush(Buf *buf)
{
  if (!buf->error && buf->count) {
    buf->error = OSWrite(buf->file, buf->data, buf->count);
    buf->count = 0;
  }
}
