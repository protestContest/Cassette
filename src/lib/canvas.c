#include "canvas.h"

#ifdef WITH_CANVAS
#include "SDL2/SDL.h"

static SDL_Renderer *renderer;
static SDL_Window *canvas;
static u32 canvas_width, canvas_height;
static bool show_canvas = false;

bool InitCanvas(u32 width, u32 height)
{
  canvas_width = width;
  canvas_height = height;

  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_CreateWindowAndRenderer(canvas_width, canvas_height, SDL_WINDOW_SHOWN, &canvas, &renderer);
  if (!canvas) return false;
  SDL_SetWindowTitle(canvas, "Canvas");
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  return true;
}

void DestroyCanvas(void)
{
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(canvas);
  SDL_Quit();
}

bool ShouldShowCanvas(void)
{
  return show_canvas;
}

void ShowCanvas(void)
{
  SDL_RenderPresent(renderer);
  bool done = false;
  SDL_Event event;
  SDL_Scancode key;

  while (!done) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_KEYDOWN:
        key = event.key.keysym.sym;
        if (key == SDLK_ESCAPE) {
          done = true;
        }
        break;
      case SDL_QUIT:
        done = true;
        break;
      }
    }
  }
}

Val CanvasLine(VM *vm, Val args)
{
  Heap *mem = &vm->mem;
  Val x1, y1, x2, y2;
  if (TupleLength(args, mem) == 4) {
    x1 = TupleGet(args, 0, mem);
    y1 = TupleGet(args, 1, mem);
    x2 = TupleGet(args, 2, mem);
    y2 = TupleGet(args, 3, mem);
  } else if (TupleLength(args, mem) == 2) {
    Val p1 = TupleGet(args, 0, mem);
    Val p2 = TupleGet(args, 1, mem);
    if (!IsTuple(p1, mem) || TupleLength(p1, mem) != 2) {
      vm->error = TypeError;
      return p1;
    }
    if (!IsTuple(p2, mem) || TupleLength(p2, mem) != 2) {
      vm->error = TypeError;
      return p2;
    }
    x1 = TupleGet(p1, 0, mem);
    y1 = TupleGet(p1, 1, mem);
    x2 = TupleGet(p2, 0, mem);
    y2 = TupleGet(p2, 1, mem);
  } else {
    vm->error = ArityError;
    return nil;
  }

  if (!IsInt(x1)) {
    vm->error = TypeError;
    return x1;
  } else if(!IsInt(y1)) {
    vm->error = TypeError;
    return y1;
  } else if (!IsInt(x2)) {
    vm->error = TypeError;
    return x2;
  } else if(!IsInt(y2)) {
    vm->error = TypeError;
    return y2;
  }

  show_canvas = true;
  SDL_RenderDrawLine(renderer, RawInt(x1), RawInt(y1), RawInt(x2), RawInt(y2));

  return SymbolFor("ok");
}

Val CanvasWidth(VM *vm, Val args)
{
  return IntVal(canvas_width);
}

Val CanvasHeight(VM *vm, Val args)
{
  return IntVal(canvas_height);
}
#endif
