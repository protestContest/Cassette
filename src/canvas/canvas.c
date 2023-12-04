#include "canvas.h"
#include "univ/str.h"
#include "univ/system.h"
#include <SDL.h>
#include <SDL_ttf.h>

#ifndef DEFAULT_FONT
/* override this in Makefile */
#define DEFAULT_FONT 0
#endif

#ifndef FONT_PATH
#define FONT_PATH 0
#endif

typedef struct {
  SDL_Window *window;
  SDL_Surface *surface;
  TTF_Font *font;
  char *font_filename;
  u32 font_size;
  u32 color;
  u32 width;
  u32 height;
} SDLCanvas;

void InitGraphics(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    perror(SDL_GetError());
    exit(1);
  }

  TTF_Init();
}

bool SetFont(Canvas *canvas, char *font_file, u32 size)
{
  TTF_Font *font;
  char *new_filename;

  if (font_file[0] != '/' && FONT_PATH) {
    new_filename = JoinPath(FONT_PATH, font_file);
  } else {
    new_filename = CopyStr(font_file, StrLen(font_file));
  }

  font = TTF_OpenFont(new_filename, size);
  if (font) {
    if (canvas->font_filename) Free(canvas->font_filename);
    if (canvas->font) TTF_CloseFont(canvas->font);
    canvas->font_filename = new_filename;
    canvas->font_size = size;
    canvas->font = font;
    return true;
  }

  return false;
}

Canvas *MakeCanvas(u32 width, u32 height, char *title)
{
  SDLCanvas *canvas = malloc(sizeof(SDLCanvas));
  if (!canvas) return 0;

  canvas->window = SDL_CreateWindow(title,
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width, height, 0);
  if (!canvas->window) return 0;
  canvas->surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  canvas->font_filename = 0;
  canvas->font = 0;
  SetFont((Canvas*)canvas, DEFAULT_FONT, 16);
  canvas->color = BLACK;
  ClearCanvas((Canvas*)canvas, WHITE);
  canvas->width = width;
  canvas->height = height;
  return (Canvas*)canvas;
}

void FreeCanvas(Canvas *canvas)
{
  if (canvas->font_filename) Free(canvas->font_filename);
  if (canvas->font) TTF_CloseFont(canvas->font);
  SDL_FreeSurface(canvas->surface);
  SDL_DestroyWindow(canvas->window);
  free(canvas);
}

void UpdateCanvas(Canvas *canvas)
{
  SDL_Surface *window = SDL_GetWindowSurface(canvas->window);
  SDL_BlitScaled(canvas->surface, 0, window, 0);
  SDL_UpdateWindowSurface(canvas->window);
}

void CanvasBlit(void *pixels, i32 x, i32 y, i32 width, i32 height, Canvas *canvas)
{
  SDL_Surface *buf = SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, 32, width*4, SDL_PIXELFORMAT_ARGB8888);
  SDL_Rect dst;
  dst.x = x;
  dst.y = y;
  SDL_BlitSurface(buf, 0, canvas->surface, &dst);
  UpdateCanvas(canvas);
  SDL_FreeSurface(buf);
}

void DrawText(char *text, i32 x, i32 y, Canvas *canvas)
{
  SDL_Surface *screen = ((SDL_Surface*)canvas->surface);
  SDL_Color color;
  SDL_Rect position;
  SDL_Surface *surface;
  if (!canvas->font) return;
  color.r = Red(canvas->color);
  color.g = Green(canvas->color);
  color.b = Blue(canvas->color);
  color.a = 0xFF;
  surface = TTF_RenderUTF8_Blended(canvas->font, text, color);
  position.x = x;
  position.y = screen->h - y - 1 - TTF_FontAscent(canvas->font);
  SDL_BlitSurface(surface, NULL, screen, &position);
  UpdateCanvas(canvas);
}

void DrawLine(i32 x0, i32 y0, i32 x1, i32 y1, Canvas *canvas)
{
  i32 dx = abs(x1 - x0);
  i32 sx = x0 < x1 ? 1 : -1;
  i32 dy = abs(y1 - y0);
  i32 sy = y0 < y1 ? 1 : -1;
  i32 error = dx - dy;
  i32 e2;
  i32 x = x0;
  i32 y = y0;

  SDL_LockSurface(canvas->surface);

  while (true) {
    WritePixel(x, y, canvas->color, canvas);
    if (x == x1 && y == y1) break;
    e2 = 2*error;
    if (e2 >= -dy) {
      if (x == x1) break;
      error -= dy;
      x += sx;
    }
    if (e2 <= dx) {
      if (y == y1) break;
      error += dx;
      y += sy;
    }
  }

  SDL_UnlockSurface(canvas->surface);

  UpdateCanvas(canvas);
}

void WritePixel(i32 x, i32 y, u32 value, Canvas *canvas)
{
  SDL_Surface *surface = canvas->surface;
  u32 *pixels = surface->pixels;
  i32 pitch = surface->pitch/4;
  i32 height = surface->h;
  if (x < 0 || x > pitch || y < 0 || y > height) return;
  pixels[(height - y - 1) * pitch + x] = value;
}

void ClearCanvas(Canvas *canvas, u32 color)
{
  SDL_Rect rect = {0};
  rect.w = ((SDL_Surface*)canvas->surface)->w;
  rect.h = ((SDL_Surface*)canvas->surface)->h;
  SDL_LockSurface(canvas->surface);
  SDL_FillRect(canvas->surface, &rect, color);
  SDL_UnlockSurface(canvas->surface);
  UpdateCanvas(canvas);
}

void MainLoop(UpdateFn update, void *arg)
{
  bool done = false;
  SDL_Event event;
  while (!done) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) done = true;
        break;
      default:
        break;
      }
    }

    if (!update(arg)) done = true;
  }

  SDL_Quit();
}
