#include "app.h"
#include "univ/font.h"
#include "univ/hash.h"
#include "univ/hashmap.h"
#include "univ/str.h"
#include "univ/system.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static ObjVec *fonts = 0;
static HashMap font_map;
static ObjVec canvases;

void InitApp(void)
{
  u32 i;

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    perror(SDL_GetError());
    exit(1);
  }

  fonts = ListFonts();
  InitHashMap(&font_map);
  for (i = 0; i < fonts->count; i++) {
    FontInfo *info = VecRef(fonts, i);
    u32 hash = Hash(info->name, StrLen(info->name));
    HashMapSet(&font_map, hash, i);
  }

  InitVec(&canvases, sizeof(Canvas*), 1);

  TTF_Init();
}

void MainLoop(UpdateFn update, void *arg)
{
  bool done = false;
  SDL_Event event;
  u32 ts = SDL_GetTicks();
  u32 frame = 30;
  u32 now;

  while (!done) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE) done = true;
        break;
      default:
        break;
      }
    }

    if (!update(arg)) done = true;

    now = SDL_GetTicks();
    if (now - ts > frame) {
      u32 i;
      ts = now;
      for (i = 0; i < canvases.count; i++) {
        UpdateCanvas(VecRef(&canvases, i));
      }
    }
  }

  SDL_Quit();
}

char *FontPath(char *name)
{
  u32 hash = Hash(name, StrLen(name));
  if (HashMapContains(&font_map, hash)) {
    FontInfo *info = VecRef(fonts, HashMapGet(&font_map, hash));
    return info->path;
  } else {
    return 0;
  }
}

void AddCanvas(Canvas *canvas)
{
  ObjVecPush(&canvases, canvas);
}
