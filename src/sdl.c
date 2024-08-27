#include "sdl.h"
#include "mem.h"
#include "univ/str.h"
#include "univ/symbol.h"
#include <SDL2/SDL.h>

Result SDLNewWindow(VM *vm)
{
  val height = VMStackPop(vm);
  val width = VMStackPop(vm);
  SDL_Window *window;
  SDL_Renderer *renderer;
  u32 ref, winref;
  val result;

  if (!IsInt(width) || !IsInt(height)) return RuntimeError("Width and height must be integers", vm);

  SDL_CreateWindowAndRenderer(RawInt(width), RawInt(height), 0, &window, &renderer);
  ref = VMPushRef(renderer, vm);
  winref = VMPushRef(window, vm);
  MaybeGC(3, vm);
  result = Tuple(2);
  TupleSet(result, 0, ref);
  TupleSet(result, 1, winref);
  return OkVal(result);
}

Result SDLDestroyWindow(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  val winref = TupleGet(refs, 1);
  SDL_Renderer *renderer = VMGetRef(RawVal(ref), vm);
  SDL_Window *window = VMGetRef(RawVal(winref), vm);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  return OkVal(0);
}

Result SDLPresent(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  SDL_Renderer *renderer = VMGetRef(RawVal(ref), vm);
  SDL_RenderPresent(renderer);
  return OkVal(0);
}

Result SDLClear(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  SDL_Renderer *renderer = VMGetRef(RawVal(ref), vm);
  SDL_RenderClear(renderer);
  return OkVal(0);
}

Result SDLLine(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  val y2 = VMStackPop(vm);
  val x2 = VMStackPop(vm);
  val y1 = VMStackPop(vm);
  val x1 = VMStackPop(vm);
  SDL_Renderer *renderer;

  if (!IsInt(x1) || !IsInt(y1) || !IsInt(x2) || !IsInt(y2)) {
    return RuntimeError("Line coords must be integers", vm);
  }

  renderer = VMGetRef(RawVal(ref), vm);
  SDL_RenderDrawLine(renderer, RawInt(x1), RawInt(y1), RawInt(x2), RawInt(y2));
  return OkVal(0);
}

Result SDLSetColor(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  val b = VMStackPop(vm);
  val g = VMStackPop(vm);
  val r = VMStackPop(vm);
  SDL_Renderer *renderer;

  if (!IsInt(r) || !IsInt(g) || !IsInt(b)) {
    return RuntimeError("Color components must be integers", vm);
  }

  renderer = VMGetRef(RawVal(ref), vm);
  SDL_SetRenderDrawColor(renderer, RawInt(r), RawInt(g), RawInt(b), SDL_ALPHA_OPAQUE);
  return OkVal(0);
}

Result SDLGetColor(VM *vm)
{
  val refs = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  SDL_Renderer *renderer = VMGetRef(RawVal(ref), vm);
  u8 r, g, b, a;
  val color;
  MaybeGC(4, vm);
  color = Tuple(3);
  SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
  TupleSet(color, 0, r);
  TupleSet(color, 0, g);
  TupleSet(color, 0, b);
  return OkVal(color);
}

static val EventType(SDL_Event *event)
{
  switch (event->type) {
  case SDL_QUIT:            return IntVal(Symbol("quit"));
  case SDL_KEYDOWN:         return IntVal(Symbol("keydown"));
  case SDL_KEYUP:           return IntVal(Symbol("keyup"));
  case SDL_MOUSEBUTTONDOWN: return IntVal(Symbol("mousedown"));
  case SDL_MOUSEBUTTONUP:   return IntVal(Symbol("mouseup"));
  default:                  return 0;
  }
}

static val KeyVal(SDL_Keycode keycode)
{
  switch (keycode) {
  case SDLK_RETURN:       return IntVal(Symbol("return"));
  case SDLK_ESCAPE:       return IntVal(Symbol("escape"));
  case SDLK_BACKSPACE:    return IntVal(Symbol("backspace"));
  case SDLK_TAB:          return IntVal(Symbol("tab"));
  case SDLK_SPACE:        return IntVal(' ');
  case SDLK_EXCLAIM:      return IntVal('!');
  case SDLK_QUOTEDBL:     return IntVal('"');
  case SDLK_HASH:         return IntVal('#');
  case SDLK_PERCENT:      return IntVal('%');
  case SDLK_DOLLAR:       return IntVal('$');
  case SDLK_AMPERSAND:    return IntVal('&');
  case SDLK_QUOTE:        return IntVal('\'');
  case SDLK_LEFTPAREN:    return IntVal('(');
  case SDLK_RIGHTPAREN:   return IntVal(')');
  case SDLK_ASTERISK:     return IntVal('*');
  case SDLK_PLUS:         return IntVal('+');
  case SDLK_COMMA:        return IntVal(',');
  case SDLK_MINUS:        return IntVal('-');
  case SDLK_PERIOD:       return IntVal('.');
  case SDLK_SLASH:        return IntVal('/');
  case SDLK_0:            return IntVal('0');
  case SDLK_1:            return IntVal('1');
  case SDLK_2:            return IntVal('2');
  case SDLK_3:            return IntVal('3');
  case SDLK_4:            return IntVal('4');
  case SDLK_5:            return IntVal('5');
  case SDLK_6:            return IntVal('6');
  case SDLK_7:            return IntVal('7');
  case SDLK_8:            return IntVal('8');
  case SDLK_9:            return IntVal('9');
  case SDLK_COLON:        return IntVal(':');
  case SDLK_SEMICOLON:    return IntVal(';');
  case SDLK_LESS:         return IntVal('<');
  case SDLK_EQUALS:       return IntVal('=');
  case SDLK_GREATER:      return IntVal('>');
  case SDLK_QUESTION:     return IntVal('?');
  case SDLK_AT:           return IntVal('@');
  case SDLK_LEFTBRACKET:  return IntVal('[');
  case SDLK_BACKSLASH:    return IntVal('\\');
  case SDLK_RIGHTBRACKET: return IntVal(']');
  case SDLK_CARET:        return IntVal('^');
  case SDLK_UNDERSCORE:   return IntVal('_');
  case SDLK_BACKQUOTE:    return IntVal('`');
  case SDLK_a:            return IntVal('a');
  case SDLK_b:            return IntVal('b');
  case SDLK_c:            return IntVal('c');
  case SDLK_d:            return IntVal('d');
  case SDLK_e:            return IntVal('e');
  case SDLK_f:            return IntVal('f');
  case SDLK_g:            return IntVal('g');
  case SDLK_h:            return IntVal('h');
  case SDLK_i:            return IntVal('i');
  case SDLK_j:            return IntVal('j');
  case SDLK_k:            return IntVal('k');
  case SDLK_l:            return IntVal('l');
  case SDLK_m:            return IntVal('m');
  case SDLK_n:            return IntVal('n');
  case SDLK_o:            return IntVal('o');
  case SDLK_p:            return IntVal('p');
  case SDLK_q:            return IntVal('q');
  case SDLK_r:            return IntVal('r');
  case SDLK_s:            return IntVal('s');
  case SDLK_t:            return IntVal('t');
  case SDLK_u:            return IntVal('u');
  case SDLK_v:            return IntVal('v');
  case SDLK_w:            return IntVal('w');
  case SDLK_x:            return IntVal('x');
  case SDLK_y:            return IntVal('y');
  case SDLK_z:            return IntVal('z');
  case SDLK_F1:           return IntVal(Symbol("f1"));
  case SDLK_F2:           return IntVal(Symbol("f2"));
  case SDLK_F3:           return IntVal(Symbol("f3"));
  case SDLK_F4:           return IntVal(Symbol("f4"));
  case SDLK_F5:           return IntVal(Symbol("f5"));
  case SDLK_F6:           return IntVal(Symbol("f6"));
  case SDLK_F7:           return IntVal(Symbol("f7"));
  case SDLK_F8:           return IntVal(Symbol("f8"));
  case SDLK_F9:           return IntVal(Symbol("f9"));
  case SDLK_F10:          return IntVal(Symbol("f10"));
  case SDLK_F11:          return IntVal(Symbol("f11"));
  case SDLK_F12:          return IntVal(Symbol("f12"));
  case SDLK_PRINTSCREEN:  return IntVal(Symbol("printscreen"));
  case SDLK_SCROLLLOCK:   return IntVal(Symbol("scrolllock"));
  case SDLK_PAUSE:        return IntVal(Symbol("pause"));
  case SDLK_INSERT:       return IntVal(Symbol("insert"));
  case SDLK_HOME:         return IntVal(Symbol("home"));
  case SDLK_PAGEUP:       return IntVal(Symbol("pageup"));
  case SDLK_DELETE:       return IntVal(Symbol("delete"));
  case SDLK_END:          return IntVal(Symbol("end"));
  case SDLK_PAGEDOWN:     return IntVal(Symbol("pagedown"));
  case SDLK_RIGHT:        return IntVal(Symbol("right"));
  case SDLK_LEFT:         return IntVal(Symbol("left"));
  case SDLK_DOWN:         return IntVal(Symbol("down"));
  case SDLK_UP:           return IntVal(Symbol("up"));
  default:                return 0;
  }
}

Result SDLPollEvent(VM *vm)
{
  SDL_Event event;
  val event_val, where;
  i32 x, y;

  MaybeGC(9, vm);
  event_val = Tuple(5);
  where = Tuple(2);

  TupleSet(event_val, 1, IntVal(event.quit.timestamp));
  SDL_GetMouseState(&x, &y);
  TupleSet(where, 0, IntVal(x));
  TupleSet(where, 1, IntVal(y));
  TupleSet(event_val, 2, where);

  if (SDL_PollEvent(&event)) {
    TupleSet(event_val, 0, EventType(&event));
    switch (event.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      TupleSet(event_val, 3, KeyVal(event.key.keysym.sym));
      TupleSet(event_val, 4, IntVal(event.key.keysym.mod));
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      TupleSet(where, 0, IntVal(event.button.x));
      TupleSet(where, 1, IntVal(event.button.y));
      break;
    }
  }
  return OkVal(event_val);
}

Result SDLGetTicks(VM *vm)
{
  return OkVal(IntVal(SDL_GetTicks()));
}

Result SDLBlit(VM *vm)
{
  val refs = VMStackPop(vm);
  val img = VMStackPop(vm);
  val width = VMStackPop(vm);
  val y = VMStackPop(vm);
  val x = VMStackPop(vm);
  val ref = TupleGet(refs, 0);
  SDL_Renderer *renderer = VMGetRef(RawVal(ref), vm);
  SDL_Texture *tex;
  void *pixels;
  u32 w, h;
  int pitch;
  SDL_Rect dst;

  if (!IsInt(y)) return RuntimeError("Expected integer", vm);
  if (!IsInt(x)) return RuntimeError("Expected integer", vm);
  if (!IsInt(width)) return RuntimeError("Expected integer", vm);
  if (!IsBinary(img)) return RuntimeError("Expected binary", vm);

  w = RawInt(width);
  h = (BinaryLength(img)*8) % w;

  dst.x = RawInt(x);
  dst.y = RawInt(y);
  dst.w = w;
  dst.h = h;

  tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_INDEX1LSB, SDL_TEXTUREACCESS_STREAMING, w, h);
  SDL_LockTexture(tex, 0, &pixels, &pitch);
  Copy(BinaryData(img), pixels, BinaryLength(img));
  SDL_UnlockTexture(tex);
  SDL_RenderCopy(renderer, tex, 0, &dst);
  SDL_DestroyTexture(tex);
  return OkVal(0);
}
