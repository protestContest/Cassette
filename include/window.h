#pragma once

typedef struct {
  char *title;
  i32 width;
  i32 height;
  u32 *buf;
  void *wnd;
} Window;

enum EventType {nullEvent, mouseDown, mouseUp, keyDown, keyUp, autoKey, quitEvent = 15};

typedef struct {
  i32 what;
  union {
    Window *window;
    i16 key;
  } message;
  i32 when;
  struct {
    i32 x;
    i32 y;
  } where;
  i32 modifiers;
} Event;

void OpenWindow(Window *window);
void CloseWindow(Window *window);
void UpdateWindow(Window *window);
void NextEvent(Event *event);
#define WritePixel(w, x, y) ((w)->buf[((y) * (w)->width) + (x)])
