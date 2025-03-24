#pragma once

#ifdef __APPLE__
#include <MacTypes.h>
#else
typedef struct {
  i16 h;
  i16 v;
} Point;

typedef struct {
  i32 left;
  i32 top;
  i32 right;
  i32 bottom;
} Rect;
#endif

typedef struct {
  i32 width;
  i32 height;
  u32 *buf;
  Point pen;
  Point origin;
  Point control;
  Point params[3];
  u32 color;
  struct {
    i16 font;
    i16 size;
  } text;
} Canvas;
#define WHITE 0x00FFFFFF
#define BLACK 0x00000000
#define PixelAt(c, x, y)  ((c)->buf[((y) * (c)->width) + (x)])
#define InCanvas(c, x, y) ((x) >= 0 && (x) < (c)->width && (y) >= 0 && (y) < (c)->height)

void InitCanvas(Canvas *canvas, i32 width, i32 height);
void DestroyCanvas(Canvas *canvas);
void SetFont(char *name, u32 size, Canvas *canvas);
void MoveTo(i32 x, i32 y, Canvas *canvas);
void Move(i32 dx, i32 dy, Canvas *canvas);
void LineTo(i32 x, i32 y, Canvas *canvas);
void Line(i32 dx, i32 dy, Canvas *canvas);
void QuadBezier(i32 cx, i32 cy, i32 x, i32 y, Canvas *canvas);
void CubicBezier(i32 cx1, i32 cy1, i32 cx2, i32 cy2, i32 x, i32 y, Canvas *canvas);
void FillRect(Rect *r, u32 color, Canvas *canvas);
void Blit(u32 *src, i32 width, i32 height, i32 x, i32 y, Canvas *canvas);
void WritePixel(i32 x, i32 y, Canvas *canvas);
