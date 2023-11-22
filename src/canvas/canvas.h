#pragma once

#ifdef CANVAS

#define WHITE   0xFFFFFFFF
#define BLACK   0x000000FF

typedef bool (*UpdateFn)(void *arg);

typedef struct {
  void *window;
  void *surface;
  void *font;
  char *font_filename;
  u32 font_size;
} Canvas;

void InitGraphics(void);
bool SetFont(Canvas *canvas, char *font_file, u32 size);
Canvas *MakeCanvas(u32 width, u32 height, char *title);
void UpdateCanvas(Canvas *canvas);
void DrawText(char *text, i32 x, i32 y, Canvas *canvas);
void DrawLine(i32 x0, i32 y0, i32 x1, i32 y1, Canvas *canvas);
void ClearCanvas(Canvas *canvas);
void WritePixel(i32 x, i32 y, u32 value, Canvas *canvas);
void MainLoop(UpdateFn update, void *arg);

#endif
