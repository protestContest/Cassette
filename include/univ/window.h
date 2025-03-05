#pragma once
#include "univ/canvas.h"

typedef struct {
  Canvas canvas;
  char *title;
  void *data;
} CTWindow;

CTWindow *NewWindow(char *title, i32 width, i32 height);

enum {nullEvent, mouseDown, mouseUp, keyDown, keyUp, autoKey, quitEvent = 15};
#ifdef __APPLE__
#else
enum {
  controlKey  = (1 << 12),
  optionKey   = (1 << 11),
  alphaLock   = (1 << 10),
  shiftKey    = (1 <<  9),
  cmdKey      = (1 <<  8),
  btnState    = (1 <<  7)
};
#endif

#define enterKey    0x03
#define escapeKey   0x1B
#define leftKey     0x1C
#define rightKey    0x1D
#define upKey       0x1E
#define downKey     0x1F

typedef struct {
  i32 what;
  union {
    CTWindow *window;
    struct {
      u8 code;
      char c;
    } key;
  } message;
  i32 when;
  struct {
    i32 x;
    i32 y;
  } where;
  i32 modifiers;
} Event;

void OpenWindow(CTWindow *window);
void CloseWindow(CTWindow *window);
void UpdateWindow(CTWindow *window);
void NextEvent(Event *event);
