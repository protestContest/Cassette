#include "primitives.h"
#include <univ/symbol.h>
#include <univ/vec.h>
#include "sdl.h"

Result VMPrint(VM *vm)
{
  val a, str;
  a = VMStackPop(vm);

  str = InspectVal(a);
  printf("%*.*s\n", BinaryLength(str), BinaryLength(str), BinaryData(str));

  return OkVal(SymVal(Symbol("ok")));
}

Result VMFormat(VM *vm)
{
  val a;
  a = VMStackPop(vm);
  return OkVal(FormatVal(a));
}

static PrimDef primitives[] = {
  {"print", VMPrint},
  {"format", VMFormat},
  {"sdl_new_window", SDLNewWindow},
  {"sdl_present", SDLPresent},
  {"sdl_clear", SDLClear},
  {"sdl_line", SDLLine},
  {"sdl_set_color", SDLSetColor},
  {"sdl_poll_event", SDLPollEvent}
};

PrimDef *Primitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

i32 PrimitiveID(u32 name)
{
  u32 i;
  for (i = 0; i < ArrayCount(primitives); i++) {
    if (Symbol(primitives[i].name) == name) return i;
  }
  return -1;
}
