#pragma once

typedef struct {
  i32 left;
  i32 top;
  i32 right;
  i32 bottom;
} BBox;

typedef struct {
  i32 width;
  i32 height;
  u32 *buf;
  struct {
    i32 x;
    i32 y;
    u32 color;
  } pen;
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
void FillRect(BBox *r, u32 color, Canvas *canvas);
void Blit(u32 *src, i32 width, i32 height, i32 x, i32 y, Canvas *canvas);
void WritePixel(i32 x, i32 y, Canvas *canvas);
