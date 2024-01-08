#include "app.h"
#include "univ/system.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

static char *font_path = DEFAULT_FONT_PATH;

void InitApp(void)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    perror(SDL_GetError());
    exit(1);
  }

  font_path = GetEnv("CASSETTE_FONTS");
  if (!font_path || !*font_path) font_path = DEFAULT_FONT_PATH;

  TTF_Init();
}

char *FontPath(void)
{
  return font_path;
}

void MainLoop(UpdateFn update, void *arg)
{
  bool done = false;
  SDL_Event event;
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
  }

  SDL_Quit();
}
