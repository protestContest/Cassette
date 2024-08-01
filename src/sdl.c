#include "sdl.h"
#include "mem.h"
#include <univ/symbol.h>
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

static val KeySymbol(SDL_Keycode keycode)
{
  switch (keycode) {
  case SDLK_RETURN:       return SymVal(Symbol("return"));
  case SDLK_ESCAPE:       return SymVal(Symbol("escape"));
  case SDLK_BACKSPACE:    return SymVal(Symbol("backspace"));
  case SDLK_TAB:          return SymVal(Symbol("tab"));
  case SDLK_SPACE:        return SymVal(Symbol("space"));
  case SDLK_EXCLAIM:      return SymVal(Symbol("exclaim"));
  case SDLK_QUOTEDBL:     return SymVal(Symbol("quotedbl"));
  case SDLK_HASH:         return SymVal(Symbol("hash"));
  case SDLK_PERCENT:      return SymVal(Symbol("percent"));
  case SDLK_DOLLAR:       return SymVal(Symbol("dollar"));
  case SDLK_AMPERSAND:    return SymVal(Symbol("ampersand"));
  case SDLK_QUOTE:        return SymVal(Symbol("quote"));
  case SDLK_LEFTPAREN:    return SymVal(Symbol("leftparen"));
  case SDLK_RIGHTPAREN:   return SymVal(Symbol("rightparen"));
  case SDLK_ASTERISK:     return SymVal(Symbol("asterisk"));
  case SDLK_PLUS:         return SymVal(Symbol("plus"));
  case SDLK_COMMA:        return SymVal(Symbol("comma"));
  case SDLK_MINUS:        return SymVal(Symbol("minus"));
  case SDLK_PERIOD:       return SymVal(Symbol("period"));
  case SDLK_SLASH:        return SymVal(Symbol("slash"));
  case SDLK_0:            return SymVal(Symbol("0"));
  case SDLK_1:            return SymVal(Symbol("1"));
  case SDLK_2:            return SymVal(Symbol("2"));
  case SDLK_3:            return SymVal(Symbol("3"));
  case SDLK_4:            return SymVal(Symbol("4"));
  case SDLK_5:            return SymVal(Symbol("5"));
  case SDLK_6:            return SymVal(Symbol("6"));
  case SDLK_7:            return SymVal(Symbol("7"));
  case SDLK_8:            return SymVal(Symbol("8"));
  case SDLK_9:            return SymVal(Symbol("9"));
  case SDLK_COLON:        return SymVal(Symbol("colon"));
  case SDLK_SEMICOLON:    return SymVal(Symbol("semicolon"));
  case SDLK_LESS:         return SymVal(Symbol("less"));
  case SDLK_EQUALS:       return SymVal(Symbol("equals"));
  case SDLK_GREATER:      return SymVal(Symbol("greater"));
  case SDLK_QUESTION:     return SymVal(Symbol("question"));
  case SDLK_AT:           return SymVal(Symbol("at"));
  case SDLK_LEFTBRACKET:  return SymVal(Symbol("leftbracket"));
  case SDLK_BACKSLASH:    return SymVal(Symbol("backslash"));
  case SDLK_RIGHTBRACKET: return SymVal(Symbol("rightbracket"));
  case SDLK_CARET:        return SymVal(Symbol("caret"));
  case SDLK_UNDERSCORE:   return SymVal(Symbol("underscore"));
  case SDLK_BACKQUOTE:    return SymVal(Symbol("backquote"));
  case SDLK_a:            return SymVal(Symbol("a"));
  case SDLK_b:            return SymVal(Symbol("b"));
  case SDLK_c:            return SymVal(Symbol("c"));
  case SDLK_d:            return SymVal(Symbol("d"));
  case SDLK_e:            return SymVal(Symbol("e"));
  case SDLK_f:            return SymVal(Symbol("f"));
  case SDLK_g:            return SymVal(Symbol("g"));
  case SDLK_h:            return SymVal(Symbol("h"));
  case SDLK_i:            return SymVal(Symbol("i"));
  case SDLK_j:            return SymVal(Symbol("j"));
  case SDLK_k:            return SymVal(Symbol("k"));
  case SDLK_l:            return SymVal(Symbol("l"));
  case SDLK_m:            return SymVal(Symbol("m"));
  case SDLK_n:            return SymVal(Symbol("n"));
  case SDLK_o:            return SymVal(Symbol("o"));
  case SDLK_p:            return SymVal(Symbol("p"));
  case SDLK_q:            return SymVal(Symbol("q"));
  case SDLK_r:            return SymVal(Symbol("r"));
  case SDLK_s:            return SymVal(Symbol("s"));
  case SDLK_t:            return SymVal(Symbol("t"));
  case SDLK_u:            return SymVal(Symbol("u"));
  case SDLK_v:            return SymVal(Symbol("v"));
  case SDLK_w:            return SymVal(Symbol("w"));
  case SDLK_x:            return SymVal(Symbol("x"));
  case SDLK_y:            return SymVal(Symbol("y"));
  case SDLK_z:            return SymVal(Symbol("z"));
  case SDLK_CAPSLOCK:     return SymVal(Symbol("capslock"));
  case SDLK_F1:           return SymVal(Symbol("f1"));
  case SDLK_F2:           return SymVal(Symbol("f2"));
  case SDLK_F3:           return SymVal(Symbol("f3"));
  case SDLK_F4:           return SymVal(Symbol("f4"));
  case SDLK_F5:           return SymVal(Symbol("f5"));
  case SDLK_F6:           return SymVal(Symbol("f6"));
  case SDLK_F7:           return SymVal(Symbol("f7"));
  case SDLK_F8:           return SymVal(Symbol("f8"));
  case SDLK_F9:           return SymVal(Symbol("f9"));
  case SDLK_F10:          return SymVal(Symbol("f10"));
  case SDLK_F11:          return SymVal(Symbol("f11"));
  case SDLK_F12:          return SymVal(Symbol("f12"));
  case SDLK_PRINTSCREEN:  return SymVal(Symbol("printscreen"));
  case SDLK_SCROLLLOCK:   return SymVal(Symbol("scrolllock"));
  case SDLK_PAUSE:        return SymVal(Symbol("pause"));
  case SDLK_INSERT:       return SymVal(Symbol("insert"));
  case SDLK_HOME:         return SymVal(Symbol("home"));
  case SDLK_PAGEUP:       return SymVal(Symbol("pageup"));
  case SDLK_DELETE:       return SymVal(Symbol("delete"));
  case SDLK_END:          return SymVal(Symbol("end"));
  case SDLK_PAGEDOWN:     return SymVal(Symbol("pagedown"));
  case SDLK_RIGHT:        return SymVal(Symbol("right"));
  case SDLK_LEFT:         return SymVal(Symbol("left"));
  case SDLK_DOWN:         return SymVal(Symbol("down"));
  case SDLK_UP:           return SymVal(Symbol("up"));
  default:                return SymVal(Symbol("unknown"));
  }
}

Result SDLPollEvent(VM *vm)
{
  SDL_Event event;
  if (SDL_PollEvent(&event)) {
    val event_val;
    switch (event.type) {
    case SDL_QUIT:
      event_val = Tuple(2);
      TupleSet(event_val, 0, SymVal(Symbol("quit")));
      TupleSet(event_val, 1, IntVal(event.quit.timestamp));
      break;
    case SDL_KEYDOWN:
      event_val = Tuple(3);
      TupleSet(event_val, 0, SymVal(Symbol("keydown")));
      TupleSet(event_val, 1, IntVal(event.key.timestamp));
      TupleSet(event_val, 2, KeySymbol(event.key.keysym.sym));
      break;
    default:
      return OkVal(0);
    }
    return OkVal(event_val);
  } else {
    return OkVal(0);
  }
}
