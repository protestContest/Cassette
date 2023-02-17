#pragma once
#include "buf.h"
#include "string.h"

extern Buf *outbuf;

void Print(char *str);
void PrintN(char *str, u32 size);
void PrintInt(u32 num, u32 size);
void PrintFloat(float num);

char *ReadLine(char *buf, u32 size);
