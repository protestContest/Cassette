#pragma once
#include "value.h"

Value CountGraphemes(Value text, Value start, Value end);
u32 RawCountGraphemes(char *text);

#define IsSpace(c)        ((c) == ' ' || (c) == '\n' || (c) == '\t' || (c) == '\r')
#define IsDigit(c)        ((c) >= '0' && (c) <= '9')
#define IsAlpha(c)        (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && c <= 'z'))
#define IsNul(c)          ((c) == '\0')
#define IsNewline(c)      ((c) == '\n' || (c) == '\r')
#define IsCtrl(c)         (((c) < 0x20) || (c) == 0x7F)
#define IsUTFContinue(c)  (((c) & 0xC0) == 0x80)
