#pragma once

#define WHITE     0xFFFFFFFF
#define BLACK     0x000000FF

#define Red(c)    ((c) >> 24)
#define Green(c)  (((c) >> 16) & 0xFF)
#define Blue(c)   (((c) >> 8) & 0xFF)

typedef bool (*UpdateFn)(void *arg);

typedef struct {
  void *window;
  void *surface;
  void *font;
  char *font_filename;
  u32 font_size;
  u32 color;
  u32 width;
  u32 height;
} Canvas;

void InitGraphics(void);
bool SetFont(Canvas *canvas, char *font_file, u32 size);
Canvas *MakeCanvas(u32 width, u32 height, char *title);
void FreeCanvas(Canvas *canvas);
void UpdateCanvas(Canvas *canvas);
void CanvasBlit(void *pixels, i32 x, i32 y, i32 width, i32 height, Canvas *canvas);
void DrawText(char *text, i32 x, i32 y, Canvas *canvas);
void DrawLine(i32 x0, i32 y0, i32 x1, i32 y1, Canvas *canvas);
void ClearCanvas(Canvas *canvas, u32 color);
void WritePixel(i32 x, i32 y, u32 value, Canvas *canvas);
void MainLoop(UpdateFn update, void *arg);
