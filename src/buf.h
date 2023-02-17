#pragma once

#define MemBuf(data, cap)       { 0, cap, data, -1, false }
#define IOBuf(file, data, cap)  { 0, cap, data, file, false }

typedef struct {
  u32 count;
  u32 capacity;
  u8 *data;
  int file;
  bool error;
} Buf;

void Append(Buf *buf, u8 *src, u32 len);
#define AppendStr(buf, str) Append(buf, (u8*)str, sizeof(str)-1)
void AppendByte(Buf *buf, u8 byte);
void AppendInt(Buf *buf, i32 num);
void AppendFloat(Buf *buf, float num);
void Flush(Buf *buf);
