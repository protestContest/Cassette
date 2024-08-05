#include "primitives.h"
#include "sdl.h"
#include <univ/symbol.h>
#include <univ/vec.h>
#include <univ/str.h>
#include <univ/math.h>
#include <univ/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

Result VMArityError(VM *vm)
{
  return RuntimeError("Wrong number of arguments", vm);
}

Result VMPrint(VM *vm)
{
  val a, str;
  a = VMStackPop(vm);

  if (IsBinary(a)) {
    str = a;
  } else {
    str = InspectVal(a);
  }
  printf("%*.*s\n", BinaryLength(str), BinaryLength(str), BinaryData(str));

  return OkVal(SymVal(Symbol("ok")));
}

Result VMFormat(VM *vm)
{
  val a;
  a = VMStackPop(vm);
  return OkVal(FormatVal(a));
}

Result VMPanic(VM *vm)
{
  val a = VMStackPop(vm);
  char *str = BinToStr(InspectVal(a));
  return RuntimeError(str, vm);
}

Result VMTypeOf(VM *vm)
{
  val a = VMStackPop(vm);
  if (IsPair(a)) return OkVal(SymVal(Symbol("pair")));
  if (IsTuple(a)) return OkVal(SymVal(Symbol("tuple")));
  if (IsBinary(a)) return OkVal(SymVal(Symbol("binary")));
  if (IsInt(a)) return OkVal(SymVal(Symbol("integer")));
  if (IsSym(a)) return OkVal(SymVal(Symbol("symbol")));
  return OkVal(SymVal(Symbol("unknown")));
}

Result VMOpen(VM *vm)
{
  val mode = VMStackPop(vm);
  val flags = VMStackPop(vm);
  val path = VMStackPop(vm);
  char *str;
  i32 file;

  if (!IsBinary(path)) return RuntimeError("Path must be a string", vm);
  if (!IsInt(flags)) return RuntimeError("Flags must be an integer", vm);
  if (!IsInt(mode)) return RuntimeError("Mode must be an integer", vm);

  str = BinToStr(path);
  file = open(str, flags, mode);
  free(str);
  return OkVal(IntVal(file));
}

Result VMClose(VM *vm)
{
  val file = VMStackPop(vm);
  u32 err;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);

  err = close(file);
  if (err != 0) {
    return OkVal(IntVal(errno));
  } else {
    return OkVal(SymVal(Symbol("ok")));
  }
}

Result VMRead(VM *vm)
{
  val size = VMStackPop(vm);
  val file = VMStackPop(vm);
  val bin;
  u8 *data;
  u32 bytes_read;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = read(RawInt(file), data, RawInt(size));
  bin = NewBinary(bytes_read);
  Copy(data, BinaryData(bin), bytes_read);
  free(data);
  return OkVal(bin);
}

Result VMWrite(VM *vm)
{
  val size = VMStackPop(vm);
  val buf = VMStackPop(vm);
  val file = VMStackPop(vm);

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsBinary(buf)) return RuntimeError("Data must be binary", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  write(RawInt(file), BinaryData(buf), RawInt(size));
  return OkVal(buf);
}

Result VMSeek(VM *vm)
{
  val whence = VMStackPop(vm);
  val offset = VMStackPop(vm);
  val file = VMStackPop(vm);
  u32 pos;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(offset)) return RuntimeError("Offset must be an integer", vm);
  if (whence == SymVal(Symbol("set"))) {
    whence = SEEK_SET;
  } else if (whence == SymVal(Symbol("cur"))) {
    whence = SEEK_CUR;
  } else if (whence == SymVal(Symbol("end"))) {
    whence = SEEK_END;
  } else {
    return RuntimeError("Whence must be :set, :cur, or :end", vm);
  }

  pos = lseek(RawInt(file), RawInt(offset), whence);
  return OkVal(IntVal(pos));
}

Result VMRandom(VM *vm)
{
  u32 r = RandomBetween(0, MaxIntVal);
  return OkVal(IntVal(r));
}

Result VMRandomBetween(VM *vm)
{
  val max = VMStackPop(vm);
  val min = VMStackPop(vm);
  if (!IsInt(min) || !IsInt(max)) return RuntimeError("Range must be integers", vm);
  return OkVal(RandomBetween(RawInt(min), RawInt(max)));
}

Result VMSeed(VM *vm)
{
  val seed = VMStackPop(vm);
  if (!IsInt(seed)) return RuntimeError("Seed must be an integer", vm);
  SeedRandom(RawInt(seed));
  return OkVal(0);
}

Result VMTime(VM *vm)
{
  return OkVal(IntVal(Time()));
}

Result VMMicrotime(VM *vm)
{
  return OkVal(IntVal(Microtime()));
}

static PrimDef primitives[] = {
  {"arity_error", VMArityError},
  {"print", VMPrint},
  {"format", VMFormat},
  {"panic!", VMPanic},
  {"typeof", VMTypeOf},
  {"open", VMOpen},
  {"close", VMClose},
  {"read", VMRead},
  {"write", VMWrite},
  {"seek", VMSeek},
  {"random", VMRandom},
  {"random_between", VMRandomBetween},
  {"seed", VMSeed},
  {"time", VMTime},
  {"microtime", VMMicrotime},
  {"sdl_new_window", SDLNewWindow},
  {"sdl_destroy_window", SDLDestroyWindow},
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
