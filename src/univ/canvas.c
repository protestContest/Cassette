#include "univ/canvas.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/font.h"

void InitCanvas(Canvas *canvas, i32 width, i32 height)
{
  u32 i;
  canvas->width = Max(0, width);
  canvas->height = Max(0, height);
  canvas->buf = malloc(sizeof(u32)*width*height);
  for (i = 0; i < (u32)width*height; i++) {
    canvas->buf[i] = WHITE;
  }
  canvas->pen.x = 0;
  canvas->pen.y = 0;
  canvas->pen.color = BLACK;
  canvas->text.font = 0;
  canvas->text.size = 12;
}

void DestroyCanvas(Canvas *canvas)
{
  free(canvas->buf);
}

void SetFont(char *name, u32 size, Canvas *canvas)
{
  i16 num = GetFNum(name);
  if (num >= 0) {
    canvas->text.font = num;
    canvas->text.size = size;
  }
}

void MoveTo(i32 x, i32 y, Canvas *canvas)
{
  canvas->pen.x = x;
  canvas->pen.y = y;
}

void Move(i32 dx, i32 dy, Canvas *canvas)
{
  canvas->pen.x += dx;
  canvas->pen.y += dy;
}

void LineTo(i32 x, i32 y, Canvas *canvas)
{
  i32 x0 = canvas->pen.x;
  i32 y0 = canvas->pen.y;
  i32 x1 = x;
  i32 y1 = y;
  i32 dx, dy, err, sx, sy, e2;

  dx = Abs(x1 - x0);
  sx = (x0 < x1) ? 1 : -1;
  dy = Abs(y1 - y0);
  sy = (y0 < y1) ? 1 : -1;
  err = ((dx > dy) ? dx : -dy) / 2;

  while (true) {
    WritePixel(x0, y0, canvas);
    if (x0 == x1 && y0 == y1) break;
    e2 = err;
    if (e2 > -dx) { err -= dy; x0 += sx; }
    if (e2 <  dy) { err += dx; y0 += sy; }
  }
}

void Line(i32 dx, i32 dy, Canvas *canvas)
{
  LineTo(canvas->pen.x + dx, canvas->pen.y + dy, canvas);
}

void FillRect(BBox *r, u32 color, Canvas *canvas)
{
  i32 x, y;
  for (y = Max(0, r->top); y <= Min(canvas->height-1, r->bottom); y++) {
    for (x = Max(0, r->left); x <= Min(canvas->width-1, r->right); x++) {
      if (InCanvas(canvas, x, y)) {
        PixelAt(canvas, x, y) = color;
      }
    }
  }
}

void Blit(u32 *pixels, i32 width, i32 height, i32 x, i32 y, Canvas *canvas)
{
  i32 sx = 0;
  i32 sy = 0;
  i32 rowWidth = width;
  i32 i;

  /* dst is outside window */
  if (x >= canvas->width) return;
  if (y >= canvas->height) return;

  /* clip src to fit in window */
  if (x < 0) {
    width += x;
    sx = -x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    sy = -y;
    y = 0;
  }
  if (x + width > canvas->width) width = canvas->width - x;
  if (y + height > canvas->height) height = canvas->height - y;

  /* nothing to copy */
  if (width <= 0 || height <= 0) return;

  /* copy each row */
  for (i = 0; i < height; i++) {
    u32 *src = pixels + (sy+i)*rowWidth + sx;
    u32 *dst = canvas->buf + (y+i)*canvas->width + x;
    Copy(src, dst, width*sizeof(u32));
  }
}

void WritePixel(i32 x, i32 y, Canvas *canvas)
{
  if (!InCanvas(canvas, x, y)) return;
  PixelAt(canvas, x, y) = canvas->pen.color;
}
