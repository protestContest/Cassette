#include "graphics/canvas.h"
#include "univ/math.h"
#include "univ/str.h"
#include "graphics/font.h"

void InitCanvas(Canvas *canvas, i32 width, i32 height)
{
  u32 i;
  canvas->width = Max(0, width);
  canvas->height = Max(0, height);
  canvas->buf = malloc(sizeof(u32)*width*height);
  for (i = 0; i < (u32)width*height; i++) {
    canvas->buf[i] = WHITE;
  }
  canvas->pen.h = 0;
  canvas->pen.v = 0;
  canvas->origin.h = 0;
  canvas->origin.v = 0;
  canvas->control.h = 0;
  canvas->control.v = 0;
  canvas->params[0].h = 0;
  canvas->params[0].v = 0;
  canvas->params[1].h = 0;
  canvas->params[1].v = 0;
  canvas->params[2].h = 0;
  canvas->params[2].v = 0;
  canvas->color = BLACK;
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
  canvas->pen.h = x;
  canvas->pen.v = y;
}

void Move(i32 dx, i32 dy, Canvas *canvas)
{
  canvas->pen.h += dx;
  canvas->pen.v += dy;
}

void LineTo(i32 x1, i32 y1, Canvas *canvas)
{
  /* Zingl 2012 */

  i32 x0 = canvas->pen.h;
  i32 y0 = canvas->pen.v;

  i32 dx = Abs(x1 - x0);
  i32 dy = -Abs(y1 - y0);
  i32 step_x = (x0 < x1) ? 1 : -1;
  i32 step_y = (y0 < y1) ? 1 : -1;
  i32 err = dx + dy;
  i32 e2;

  while (true) {
    WritePixel(x0, y0, canvas);
    e2 = 2*err;
    if (e2 >= dy) {
      if (x0 == x1) break;
      err += dy;
      x0 += step_x;
    }
    if (e2 <= dx) {
      if (y0 == y1) break;
      err += dx;
      y0 += step_y;
    }
  }

  MoveTo(x1, y1, canvas);
}

void Line(i32 dx, i32 dy, Canvas *canvas)
{
  LineTo(canvas->pen.h + dx, canvas->pen.v + dy, canvas);
}

void QuadBezier(i32 x1, i32 y1, i32 x2, i32 y2, Canvas *canvas)
{
  i32 x0 = canvas->pen.h;
  i32 y0 = canvas->pen.v;
  i32 m01x, m01y, m12x, m12y, mx, my;

  if (Abs(x2 - x0) <= 2 && Abs(y2 - y0) <= 2) {
    MoveTo(x0, y0, canvas);
    LineTo(x2, y2, canvas);
    return;
  }

  m01x = x0 + (x1 - x0)/2;
  m01y = y0 + (y1 - y0)/2;
  m12x = x1 + (x2 - x1)/2;
  m12y = y1 + (y2 - y1)/2;
  mx = m01x + (m12x - m01x)/2;
  my = m01y + (m12y - m01y)/2;

  QuadBezier(m01x, m01y, mx, my, canvas);
  QuadBezier(m12x, m12y, x2, y2, canvas);
}

static void CubicBezierN(i32 n, i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, Canvas *canvas)
{
  i32 x0 = canvas->pen.h;
  i32 y0 = canvas->pen.v;
  i32 m01x, m01y, m12x, m12y, m23x, m23y, ax, ay, bx, by, mx, my;

  if (n == 0) {
    MoveTo(x0, y0, canvas);
    LineTo(x3, y3, canvas);
    return;
  }

  m01x = x0 + (x1 - x0)/2;
  m01y = y0 + (y1 - y0)/2;
  m12x = x1 + (x2 - x1)/2;
  m12y = y1 + (y2 - y1)/2;
  m23x = x2 + (x3 - x2)/2;
  m23y = y2 + (y3 - y2)/2;

  ax = m01x + (m12x - m01x)/2;
  ay = m01y + (m12y - m01y)/2;

  bx = m12x + (m23x - m12x)/2;
  by = m12y + (m23y - m12y)/2;

  mx = ax + (bx - ax)/2;
  my = ay + (by - ay)/2;

  CubicBezierN(n - 1, m01x, m01y, ax, ay, mx, my, canvas);
  CubicBezierN(n - 1, bx, by, m23x, m23y, x3, y3, canvas);
}

void CubicBezier(i32 x1, i32 y1, i32 x2, i32 y2, i32 x3, i32 y3, Canvas *canvas)
{
  CubicBezierN(5, x1, y1, x2, y2, x3, y3, canvas);
}

void FillRect(Rect *r, u32 color, Canvas *canvas)
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
  PixelAt(canvas, x, y) = canvas->color;
}
