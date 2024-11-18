#pragma once

typedef struct {
  char **title;
  i32 width;
  i32 height;
  u32 *buf;
  void *data;
} CTWindow;

#ifdef __APPLE__
#include <Carbon/Carbon.h>
enum {quitEvent = 15};
#else
enum {nullEvent, mouseDown, mouseUp, keyDown, keyUp, autoKey, quitEvent = 15};
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
    CTWindow **window;
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

void OpenWindow(CTWindow **window);
void CloseWindow(CTWindow **window);
void UpdateWindow(CTWindow **window);
void NextEvent(Event *event);
#define WritePixel(w, x, y) ((*(w))->buf[((y) * (*(w))->width) + (x)])
