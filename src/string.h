#pragma once

u32 StringLength(char *src, u32 start, u32 end);

#define IsCtrl(c)         (((c) < 0x20) || (c) == 0x7F)
#define IsUTFContinue(c)  (((c) & 0xC0) == 0x80)

char *CtrlChar(char c);
void ExplicitPrint(char *str, u32 count);
