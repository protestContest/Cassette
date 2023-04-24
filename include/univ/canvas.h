#pragma once

typedef struct {
  u8 b;
  u8 g;
  u8 r;
  u8 a;
} Pixel;

#define white   ((Pixel){0xFF, 0xFF, 0xFF, 0xFF})
#define black   ((Pixel){0x00, 0x00, 0x00, 0xFF})

typedef struct {
  i32 width;
  i32 height;
  Pixel *pixels;
  Pixel foreground;
  Pixel background;
} Canvas;

void InitCanvas(Canvas *canvas, i32 width, i32 height, void *pixels);
void ClearCanvas(Canvas *canvas);
void ForegroundColor(Canvas *canvas, Pixel color);
void BackgroundColor(Canvas *canvas, Pixel color);

void SetPixel(Canvas *canvas, i32 x, i32 y, Pixel color);
Pixel GetPixel(Canvas *canvas, i32 x, i32 y);

void DrawLine(Canvas *canvas, i32 x1, i32 y1, i32 x2, i32 y2);

void DrawRect(Canvas *canvas, float x1, float y1, float x2, float y2);
void FillRect(Canvas *canvas, float x1, float y1, float x2, float y2);

void DrawBezier(Canvas *canvas, float x0, float y0, float x1, float y1, float x2, float y2);
void DrawCubicBezier(Canvas *canvas, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3);
#pragma once

