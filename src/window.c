#include "window.h"
#include "univ/hash.h"
#include "univ/hashmap.h"
#include "univ/vec.h"

enum {running, quitting, done};

static CTWindow **windows = 0;
static HashMap window_map = EmptyHashMap;
static i32 state = running;

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>

#define msg(r, o, s) ((r(*)(id, SEL))objc_msgSend)(o, sel_getUid(s))
#define msg1(r, o, s, A, a)                                                    \
  ((r(*)(id, SEL, A))objc_msgSend)(o, sel_getUid(s), a)
#define msg2(r, o, s, A, a, B, b)                                              \
  ((r(*)(id, SEL, A, B))objc_msgSend)(o, sel_getUid(s), a, b)
#define msg3(r, o, s, A, a, B, b, C, c)                                        \
  ((r(*)(id, SEL, A, B, C))objc_msgSend)(o, sel_getUid(s), a, b, c)
#define msg4(r, o, s, A, a, B, b, C, c, D, d)                                  \
  ((r(*)(id, SEL, A, B, C, D))objc_msgSend)(o, sel_getUid(s), a, b, c, d)

#define cls(x) ((id)objc_getClass(x))

extern id const NSDefaultRunLoopMode;
extern id const NSApp;

static void DrawRect(id view, SEL s, CGRect r)
{
  CTWindow *window;
  CGContextRef port;
  id context;
  CGColorSpaceRef space;
  CGDataProviderRef provider;
  CGImageRef img;
  (void)r, (void)s;
  window = (CTWindow*)objc_getAssociatedObject(view, "window_data");
  context = msg(id, cls("NSGraphicsContext"), "currentContext");
  port = msg(CGContextRef, context, "graphicsPort");
  CGContextSetInterpolationQuality(port, kCGInterpolationNone);
  space = CGColorSpaceCreateDeviceRGB();
  provider = CGDataProviderCreateWithData(
    NULL, window->buf, window->width * window->height * 4, NULL);
  img =
    CGImageCreate(window->width, window->height, 8, 32, window->width * 4, space,
                  kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little,
                  provider, NULL, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(space);
  CGDataProviderRelease(provider);
  CGContextDrawImage(port, CGRectMake(0, 0, window->width, window->height), img);
  CGImageRelease(img);
}

static BOOL WindowShouldClose(id v, SEL s, id w)
{
  (void)v, (void)s, (void)w;
  return YES;
}

enum {TerminateNow = 1, TerminateCancel = 0, TerminateLater = 2};
static int ShouldTerminate(id v, SEL s, id w)
{
  state = quitting;
  return TerminateCancel;
}

void OpenWindow(CTWindow *window)
{
  Class delegateclass, viewclass;
  id title, view, delegate, ev;
  u32 key;
  msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, NSApp, "setActivationPolicy:", NSInteger, 0);
  window->data = msg4(id, msg(id, cls("NSWindow"), "alloc"),
                "initWithContentRect:styleMask:backing:defer:", CGRect,
                CGRectMake(0, 0, window->width, window->height), NSUInteger, 3,
                NSUInteger, 2, BOOL, NO);
  delegateclass =
      objc_allocateClassPair((Class)cls("NSObject"), "CassetteDelegate", 0);
  class_addMethod(delegateclass, sel_getUid("windowShouldClose:"),
                  (IMP)WindowShouldClose, "c@:@");
  class_addMethod(delegateclass, sel_getUid("applicationShouldTerminate:"),
                  (IMP)ShouldTerminate, "c@:@");
  objc_registerClassPair(delegateclass);

  delegate = msg(id, msg(id, (id)delegateclass, "alloc"), "init");
  msg1(void, window->data, "setDelegate:", id, delegate);
  viewclass = objc_allocateClassPair((Class)cls("NSView"), "CassetteView", 0);
  class_addMethod(viewclass, sel_getUid("drawRect:"),
                  (IMP)DrawRect, "i@:@@");
  objc_registerClassPair(viewclass);

  view = msg(id, msg(id, (id)viewclass, "alloc"), "init");
  msg1(void, window->data, "setContentView:", id, view);
  objc_setAssociatedObject(view, "window_data", (id)window,
                           OBJC_ASSOCIATION_ASSIGN);

  title = msg1(id, cls("NSString"), "stringWithUTF8String:", const char *,
               window->title);
  msg1(void, window->data, "setTitle:", id, title);
  msg1(void, window->data, "makeKeyAndOrderFront:", id, nil);
  msg(void, window->data, "center");
  msg1(void, NSApp, "activateIgnoringOtherApps:", BOOL, YES);

  ev = msg4(id, NSApp, "nextEventMatchingMask:untilDate:inMode:dequeue:",
    NSUInteger, NSUIntegerMax, id, NULL, id, NSDefaultRunLoopMode, BOOL, YES);
  if (ev) msg1(void, NSApp, "sendEvent:", id, ev);



  key = Hash(&window->data, sizeof(id));
  HashMapSet(&window_map, key, VecCount(windows));
  VecPush(windows, window);
}

void CloseWindow(CTWindow *window)
{
  u32 key = Hash(&window->data, sizeof(id));
  windows[HashMapGet(&window_map, key)] = 0;
  HashMapDelete(&window_map, key);
  msg(void, window->data, "close");
}

void UpdateWindow(CTWindow *window)
{
  msg1(void, msg(id, window->data, "contentView"), "setNeedsDisplay:", BOOL, YES);
}

void NextEvent(Event *event)
{
  id ev, win;
  u32 key;
  NSUInteger mod, evtype, len;
  CGPoint xy;
  CFTimeInterval timestamp;
  id chars;
  Event _;

  if (!event) event = &_;

  event->what = nullEvent;
  event->message.window = 0;
  event->when = 0;
  event->where.x = 0;
  event->where.y = 0;
  event->modifiers = 0;

  if (state == quitting) {
    event->what = quitEvent;
    state = done;
    return;
  }

  ev = msg4(id, NSApp, "nextEventMatchingMask:untilDate:inMode:dequeue:",
    NSUInteger, NSUIntegerMax, id, NULL, id, NSDefaultRunLoopMode, BOOL, YES);
  if (!ev) return;

  evtype = msg(NSUInteger, ev, "type");

  timestamp = msg(CFTimeInterval, ev, "timestamp");
  event->when = (i32)(timestamp * 1000);
  win = msg(id, ev, "window");
  key = Hash(&win, sizeof(id));
  if (HashMapContains(&window_map, key)) {
    CTWindow *window = windows[HashMapGet(&window_map, key)];
    event->message.window = window;
    msg1(void, msg(id, window->data, "contentView"), "setNeedsDisplay:", BOOL, YES);
  }

  mod = msg(NSUInteger, ev, "modifierFlags");
  event->modifiers |= (mod & (1<<16)) >> 6; /* caps lock */
  event->modifiers |= (mod & (1<<17)) >> 8; /* shift */
  event->modifiers |= (mod & (1<<18)) >> 6; /* control */
  event->modifiers |= (mod & (1<<19)) >> 8; /* option */
  event->modifiers |= (mod & (1<<20)) >> 12; /* command */
  mod = msg(NSUInteger, cls("NSEvent"), "pressedMouseButtons");
  event->modifiers |= (mod & 1) << 7; /* mouse state */

  switch (evtype) {
  case 1:
    event->what = mouseDown;
    xy = msg(CGPoint, ev, "locationInWindow");
    event->where.x = (int)xy.x;
    event->where.y = (int)xy.y;
    event->modifiers |= 1 << 7;
    break;
  case 2:
    event->what = mouseUp;
    xy = msg(CGPoint, ev, "locationInWindow");
    event->where.x = (int)xy.x;
    event->where.y = (int)xy.y;
    event->modifiers &= ~(1 << 7);
    break;
  case 10:
    event->what = keyDown;
    event->message.key.code = (u8)msg(NSUInteger, ev, "keyCode");
    chars = msg(id, ev, "characters");
    len = msg(NSUInteger, chars, "length");
    if (len > 0) {
      event->message.key.c = (u8)msg1(u16, chars, "characterAtIndex:", NSUInteger, 0);
    } else {
      event->message.key.c = 0;
    }
    return;
  case 11:
    event->what = keyUp;
    event->message.key.code = (u8)msg(NSUInteger, ev, "keyCode");
    chars = msg(id, ev, "characters");
    len = msg(NSUInteger, chars, "length");
    if (len > 0) {
      event->message.key.c = (u8)msg1(u16, chars, "characterAtIndex:", NSUInteger, 0);
    } else {
      event->message.key.c = 0;
    }
    return;
  }
  msg1(void, NSApp, "sendEvent:", id, ev);
}
#else
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

static Display *display = 0;

typedef struct {
  Window win;
  GC gc;
  XImage *img;
} XInfo;

void OpenWindow(CTWindow *window)
{
  XInfo *info = malloc(sizeof(XInfo));
  u32 key, screen;
  if (!display) display = XOpenDisplay(NULL);

  window->data = info;
  screen = DefaultScreen(display);
  info->win = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0,
      window->width, window->height, 0, BlackPixel(display, screen),
      WhitePixel(display, screen));

  info->gc = XCreateGC(display, info->win, 0, 0);
  XSelectInput(display, info->win,
      KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);
  XStoreName(display, info->win, window->title);
  XMapWindow(display, info->win);
  XSync(display, info->win);
  info->img = XCreateImage(display, DefaultVisual(display, 0), 24, ZPixmap, 0,
      (char *)window->buf, window->width, window->height, 32, 0);

  key = Hash(&info->win, sizeof(info->win));
  HashMapSet(&window_map, key, VecCount(windows));
  VecPush(windows, window);
}

void CloseWindow(CTWindow *window)
{
  XInfo *info = window->data;
  u32 key = Hash(&info->win, sizeof(info->win));
  windows[HashMapGet(&window_map, key)] = 0;
  HashMapDelete(&window_map, key);
  XDestroyWindow(display, info->win);
  free(info);
}

void UpdateWindow(CTWindow *window)
{
  XInfo *info = window->data;
  XPutImage(display, info->win, info->gc, info->img, 0, 0, 0, 0, window->width,
      window->height);
  XFlush(display);
}

static u32 Modifiers(u32 state)
{
  u32 mod = 0;
  mod |= (state & LockMask) << 9; /* caps lock */
  mod |= (state & ShiftMask) << 9; /* shift */
  mod |= (state & ControlMask) << 10; /* control */
  mod |= (state & Mod1Mask) << 8; /* alt */
  mod |= (state & Mod4Mask) << 2; /* meta */
  mod |= (state & Button1Mask) >> 1; /* mouse state */
  return mod;
}

static u8 KeyToChar(u32 key)
{
  if (key < 128) return (u8)key;
  switch (key) {
  case XK_KP_Enter:   return enterKey;
  case XK_BackSpace:  return '\b';
  case XK_Tab:        return '\t';
  case XK_Linefeed:   return '\n';
  case XK_Return:     return '\r';
  case XK_Escape:     return escapeKey;
  case XK_Left:       return leftKey;
  case XK_Right:      return rightKey;
  case XK_Up:         return upKey;
  case XK_Down:       return downKey;
  default:            return 0;
  }
}

void NextEvent(Event *event)
{
  XEvent ev;
  u32 key;

  event->what = nullEvent;
  event->message.window = 0;
  event->when = 0;
  event->where.x = 0;
  event->where.y = 0;
  event->modifiers = 0;

  if (state == quitting) {
    event->what = quitEvent;
    state = done;
    return;
  }

  XNextEvent(display, &ev);

  key = Hash(&ev.xany.window, sizeof(ev.xany.window));
  if (HashMapContains(&window_map, key)) {
    CTWindow *window = windows[HashMapGet(&window_map, key)];
    event->message.window = window;
    UpdateWindow(window);
  }

  switch (ev.type) {
  case ButtonPress:
    event->what = mouseDown;
    event->when = ev.xbutton.time;
    event->where.x = ev.xbutton.x;
    event->where.y = ev.xbutton.y;
    event->modifiers = Modifiers(ev.xbutton.state);
    break;
  case ButtonRelease:
    event->what = mouseUp;
    event->when = ev.xbutton.time;
    event->where.x = ev.xbutton.x;
    event->where.y = ev.xbutton.y;
    event->modifiers = Modifiers(ev.xbutton.state);
    break;
  case KeyPress:
    event->what = keyDown;
    event->when = ev.xkey.time;
    event->message.key.code = ev.xkey.keycode;
    key = XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0);
    event->message.key.c = KeyToChar(key);
    event->modifiers = Modifiers(ev.xbutton.state);
    break;
  case KeyRelease:
    event->what = keyUp;
    event->when = ev.xkey.time;
    event->message.key.code = ev.xkey.keycode;
    key = XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0);
    event->message.key.c = KeyToChar(key);
    event->modifiers = Modifiers(ev.xbutton.state);
    break;
  }
}

#endif
