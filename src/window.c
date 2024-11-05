#include "window.h"
#include "univ/hash.h"
#include "univ/hashmap.h"
#include "univ/vec.h"

#include <CoreGraphics/CoreGraphics.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc-runtime.h>

enum {running, quitting, done};

static Window **windows = 0;
static HashMap window_map = EmptyHashMap;
static i32 state = running;

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

/* msg1(void, NSApp, "terminate:", id, NSApp); */

static void DrawRect(id view, SEL s, CGRect r)
{
  Window *window;
  CGContextRef port;
  id context;
  CGColorSpaceRef space;
  CGDataProviderRef provider;
  CGImageRef img;
  (void)r, (void)s;
  window = (Window*)objc_getAssociatedObject(view, "window_data");
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

void OpenWindow(Window *window)
{
  Class delegateclass, viewclass;
  id title, view, delegate;
  uint32_t key;
  msg(id, cls("NSApplication"), "sharedApplication");
  msg1(void, NSApp, "setActivationPolicy:", NSInteger, 0);
  window->wnd = msg4(id, msg(id, cls("NSWindow"), "alloc"),
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
  msg1(void, window->wnd, "setDelegate:", id, delegate);
  viewclass = objc_allocateClassPair((Class)cls("NSView"), "CassetteView", 0);
  class_addMethod(viewclass, sel_getUid("drawRect:"),
                  (IMP)DrawRect, "i@:@@");
  objc_registerClassPair(viewclass);

  view = msg(id, msg(id, (id)viewclass, "alloc"), "init");
  msg1(void, window->wnd, "setContentView:", id, view);
  objc_setAssociatedObject(view, "window_data", (id)window,
                           OBJC_ASSOCIATION_ASSIGN);

  title = msg1(id, cls("NSString"), "stringWithUTF8String:", const char *,
               window->title);
  msg1(void, window->wnd, "setTitle:", id, title);
  msg1(void, window->wnd, "makeKeyAndOrderFront:", id, nil);
  msg(void, window->wnd, "center");
  msg1(void, NSApp, "activateIgnoringOtherApps:", BOOL, YES);

  key = Hash(&window->wnd, sizeof(id));
  HashMapSet(&window_map, key, VecCount(windows));
  VecPush(windows, window);
}

void CloseWindow(Window *window)
{
  uint32_t key = Hash(&window->wnd, sizeof(id));
  windows[HashMapGet(&window_map, key)] = 0;
  HashMapDelete(&window_map, key);
  msg(void, window->wnd, "close");
}

void UpdateWindow(Window *window)
{
  msg1(void, msg(id, window->wnd, "contentView"), "setNeedsDisplay:", BOOL, YES);
}

static char KEYCODES[] = {
  'a', 's', 'd', 'f', 'h', 'g', 'z', 'x', 'c', 'v', 0, 'b', 'q', 'w', 'e', 'r',
  'y', 't', '1', '2', '3', '4', '6', '5', '=', '9', '7', '-', '8', '0', ']', 'o',
  'u', '[', 'i', 'p', 0, 'l', 'j', '"', 'k', ';', '\\', ',', '/', 'n', 'm', '.',
  0, 0, '`'
};

void NextEvent(Event *event)
{
  id ev, win;
  u32 key;
  char c;
  NSUInteger mod, evtype, k;
  CGPoint xy;
  CFTimeInterval timestamp;
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

  ev = msg4(id, NSApp,
               "nextEventMatchingMask:untilDate:inMode:dequeue:", NSUInteger,
               NSUIntegerMax, id, NULL, id, NSDefaultRunLoopMode, BOOL, YES);
  if (!ev) return;

  evtype = msg(NSUInteger, ev, "type");

  timestamp = msg(CFTimeInterval, ev, "timestamp");
  event->when = (i32)(timestamp * 1000);
  win = msg(id, ev, "window");
  key = Hash(&win, sizeof(id));
  if (HashMapContains(&window_map, key)) {
    Window *window = windows[HashMapGet(&window_map, key)];
    event->message.window = window;
    msg1(void, msg(id, window->wnd, "contentView"), "setNeedsDisplay:", BOOL, YES);
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
    k = msg(NSUInteger, ev, "keyCode");
    c = k < ArrayCount(KEYCODES) ? KEYCODES[k] : 0;
    event->message.key = (k << 8) | c;
    break;
  case 11:
    event->what = keyUp;
    k = msg(NSUInteger, ev, "keyCode");
    c = k < ArrayCount(KEYCODES) ? KEYCODES[k] : 0;
    event->message.key = (k << 8) | c;
    break;
  }
  msg1(void, NSApp, "sendEvent:", id, ev);
}
