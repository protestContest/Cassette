/* event.h */
#pragma once

// #define DEBUG_EVENTS

typedef enum {
  NullEvent,
  MouseDownEvent,
  MouseUpEvent,
  KeyDownEvent,
  KeyUpEvent,
  AutoKeyEvent,
  UpdateEvent,
  DiskEvent,
  ActivateEvent,
  NetworkEvent,
  DeviceEvent,
  QuitEvent,
} EventType;
#define NumEventTypes 12

#define modNone     0
#define modActive   (1 << 0)
#define modButton   (1 << 7)
#define modCommand  (1 << 8)
#define modShift    (1 << 9)
#define modCapslock (1 << 10)
#define modOption   (1 << 11)

typedef struct {
  EventType type;
  union {
    struct {
      char key;
      u8 code;
    };
    struct Window *window;
    u32 drive;
  };
  u32 when;
  struct {
    i32 x;
    i32 y;
  } where;
  u16 modifiers;
} Event;

typedef u32 EventHandlerID;
typedef void (*EventHandler)(Event event);

EventHandlerID AddEventHandler(EventType type, EventHandler handler);
void RemoveEventHandler(EventHandlerID id);

void HandleEvents(void);
Event WaitNextEvent(EventType type);
void DispatchEvent(Event event);
void QueueEvent(Event event);
u32 TickCount(void);

void Quit(void);
/* keycodes.h */
#pragma once
#include <SDL2/SDL.h>

typedef enum {
  EscapeKey     = 0x35,
  OneKey        = 0x12,
  TwoKey        = 0x13,
  ThreeKey      = 0x14,
  FourKey       = 0x15,
  FiveKey       = 0x17,
  SixKey        = 0x16,
  SevenKey      = 0x1A,
  EightKey      = 0x1C,
  NineKey       = 0x19,
  ZeroKey       = 0x1D,
  DashKey       = 0x1B,
  EqualKey      = 0x18,
  DeleteKey     = 0x33,
  TabKey        = 0x30,
  QKey          = 0x0C,
  WKey          = 0x0D,
  EKey          = 0x0E,
  RKey          = 0x0F,
  TKey          = 0x11,
  YKey          = 0x10,
  UKey          = 0x20,
  IKey          = 0x22,
  OKey          = 0x1F,
  PKey          = 0x23,
  LBracketKey   = 0x21,
  RBracketKey   = 0x1E,
  ControlKey    = 0x3B,
  AKey          = 0x00,
  SKey          = 0x01,
  DKey          = 0x02,
  FKey          = 0x03,
  GKey          = 0x05,
  HKey          = 0x04,
  JKey          = 0x26,
  KKey          = 0x28,
  LKey          = 0x25,
  SemicolonKey  = 0x29,
  QuoteKey      = 0x27,
  ReturnKey     = 0x24,
  ShiftKey      = 0x38,
  ZKey          = 0x06,
  XKey          = 0x07,
  CKey          = 0x08,
  VKey          = 0x09,
  BKey          = 0x0B,
  NKey          = 0x2D,
  MKey          = 0x2E,
  CommaKey      = 0x2B,
  PeriodKey     = 0x2F,
  SlashKey      = 0x2C,
  CapslockKey   = 0x39,
  OptionKey     = 0x3A,
  LCommandKey   = 0x37,
  BackslashKey  = 0x2A,
  SpaceKey      = 0x31,
  LeftKey       = 0x7B,
  RightKey      = 0x7C,
  DownKey       = 0x7D,
  UpKey         = 0x7E,
  NumClearKey   = 0x47,
  NumEqualKey   = 0x51,
  NumSlashKey   = 0x4B,
  NumStarKey    = 0x43,
  Num7Key       = 0x59,
  Num8Key       = 0x5B,
  Num9Key       = 0x5C,
  NumPlusKey    = 0x45,
  Num4Key       = 0x56,
  Num5Key       = 0x57,
  Num6Key       = 0x58,
  NumMinusKey   = 0x4E,
  Num1Key       = 0x53,
  Num2Key       = 0x54,
  Num3Key       = 0x55,
  NumEnterKey   = 0x4C,
  Num0Key       = 0x52,
  NumDotKey     = 0x41,
  UnknownKey    = 0xFF,
} KeyCode;

KeyCode KeyCodeFromSDL(SDL_Scancode sdl_code);
SDL_Scancode KeyCodeToSDL(KeyCode code);
/* window.h */
#pragma once
#include <univ/canvas.h>

typedef struct Window {
  Canvas canvas;
  struct Window *next_window;
  struct {
    void *a;
    void *b;
  } details;
} Window;

Window *NewWindow(char *title, u32 width, u32 height);
void FreeWindow(Window *window);

void PresentWindow(Window *window);
void RedrawWindow(Window *window);
Window *FrontWindow(void);
