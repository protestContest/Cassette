#pragma once
#include "univ/canvas.h"

typedef struct {
  i16 ascent;
  i16 descent;
  i16 widMax;
  i16 leading;
} FontInfo;

void DrawChar(char ch, Canvas *canvas);
void DrawString(char *str, Canvas *canvas);
i32 StringWidth(char *str, Canvas *canvas);
i16 GetFNum(char *name);
void GetFontInfo(FontInfo *info, Canvas *canvas);
void ListFonts(void);
void DumpFont(Canvas *canvas);
