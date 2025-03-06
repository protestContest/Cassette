#pragma once
#include "univ/canvas.h"

void DrawChar(char ch, Canvas *canvas);
void DrawString(char *str, Canvas *canvas);
i32 StringWidth(char *str, Canvas *canvas);
i16 GetFNum(char *name);
void ListFonts(void);
void DumpFont(Canvas *canvas);
