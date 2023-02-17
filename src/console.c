#include "console.h"
#include "platform/io.h"

static u8 bufdata[1024];
static Buf stdout_buf = OutputBuf(bufdata, 1024);
Buf *outbuf = &stdout_buf;

void Print(char *str)
{
  Append(outbuf, (u8*)str, StrLen(str));
  Flush(outbuf);
}

void PrintN(char *str, u32 size)
{
  u32 len = StrLen(str);
  size = (len > size) ? len : size;
  for (u32 i = 0; i < size - len; i++) AppendByte(outbuf, ' ');
  Append(outbuf, (u8*)str, len);
  Flush(outbuf);
}

void PrintInt(u32 num, u32 size)
{
  u32 digits = NumDigits(num);
  size = (digits > size) ? digits : size;
  for (u32 i = 0; i < size - digits; i++) AppendByte(outbuf, '0');
  AppendInt(outbuf, num);
  Flush(outbuf);
}

void PrintFloat(float num)
{
  AppendFloat(outbuf, num);
}

char *ReadLine(char *buf, u32 size)
{
  return NULL;
  // return fgets(buf, size, stdin);
}
