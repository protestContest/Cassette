#include "string.h"

char CharAt(Value text, Value index)
{
  return BinaryData(text)[RawVal(index)];
}

Value CountGraphemes(Value text, Value start, Value end)
{
  Value len = IndexVal(0);

  if (IsLessThan(start, end) || IsEqual(start, end)) {
    for (Value i = start; IsLessThan(i, end); i = Incr(i)) {
      if (!IsUTFContinue(CharAt(text, i))) {
        len = Incr(len);
      }
    }
  }

  return len;
}

u32 RawCountGraphemes(char *text)
{
  u32 len = 0;

  while (*text != '\0') {
    if (!IsUTFContinue(*text)) len++;
    text++;
  }

  return len;
}

char *CtrlChar(char c)
{
  switch (c) {
  case 0x00:  return "␀";
  case 0x01:  return "␁";
  case 0x02:  return "␂";
  case 0x03:  return "␃";
  case 0x04:  return "␄";
  case 0x05:  return "␅";
  case 0x06:  return "␆";
  case 0x07:  return "␇";
  case 0x08:  return "␈";
  case 0x09:  return "␉";
  case 0x0A:  return "␊";
  case 0x0B:  return "␋";
  case 0x0C:  return "␌";
  case 0x0D:  return "␍";
  case 0x0E:  return "␎";
  case 0x0F:  return "␏";
  case 0x10:  return "␐";
  case 0x11:  return "␑";
  case 0x12:  return "␒";
  case 0x13:  return "␓";
  case 0x14:  return "␔";
  case 0x15:  return "␕";
  case 0x16:  return "␖";
  case 0x17:  return "␗";
  case 0x18:  return "␘";
  case 0x19:  return "␙";
  case 0x1A:  return "␚";
  case 0x1B:  return "␛";
  case 0x1C:  return "␜";
  case 0x1D:  return "␝";
  case 0x1E:  return "␞";
  case 0x1F:  return "␟";
  case 0x7F:  return "␡";
  default:    return "";
  }
}
