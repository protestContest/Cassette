#include "primitives.h"
#include "sdl.h"
#include "univ/symbol.h"
#include "univ/vec.h"
#include "univ/str.h"
#include "univ/math.h"
#include "univ/time.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static val IOError(VM *vm);

Result VMArityError(VM *vm)
{
  UnwindVM(vm);
  return RuntimeError("Wrong number of arguments", vm);
}

Result VMPanic(VM *vm)
{
  val a = VMStackPop(vm);
  char *str = BinToStr(InspectVal(a));
  Result result = RuntimeError(str, vm);
  free(str);
  return result;
}

Result VMTypeOf(VM *vm)
{
  val a = VMStackPop(vm);
  if (IsPair(a)) return OkVal(IntVal(Symbol("pair")));
  if (IsTuple(a)) return OkVal(IntVal(Symbol("tuple")));
  if (IsBinary(a)) return OkVal(IntVal(Symbol("binary")));
  if (IsInt(a)) {
    char *name = SymbolName(RawVal(a));
    if (name) return OkVal(IntVal(Symbol("symbol")));
    return OkVal(IntVal(Symbol("integer")));
  }
  return OkVal(IntVal(Symbol("unknown")));
}

Result VMByte(VM *vm)
{
  val a = VMStackPop(vm);
  u32 num;
  val bin;
  if (!IsInt(a)) return RuntimeError("Bad byte value", vm);
  num = RawInt(a);
  if (num < 0 || num > 255) return RuntimeError("Bad byte value", vm);
  MaybeGC(2, vm);
  bin = NewBinary(1);
  BinaryData(bin)[0] = num;
  return OkVal(bin);
}

Result VMSymbolName(VM *vm)
{
  val a = VMStackPop(vm);
  char *name = SymbolName(RawVal(a));
  val bin;
  if (!name) return OkVal(0);
  MaybeGC(BinSpace(strlen(name)) + 1, vm);
  bin = BinaryFrom(name, strlen(name));
  return OkVal(bin);
}

Result VMOpen(VM *vm)
{
  val flags = VMStackPop(vm);
  val path = VMStackPop(vm);
  char *str;
  i32 file;

  if (!IsBinary(path)) return RuntimeError("Path must be a string", vm);
  if (!IsInt(flags)) return RuntimeError("Flags must be an integer", vm);

  str = BinToStr(path);
  file = open(str, RawInt(flags), 0x1FF);
  free(str);
  if (file == -1) return OkVal(IOError(vm));
  return OkVal(IntVal(file));
}

Result VMClose(VM *vm)
{
  val file = VMStackPop(vm);
  u32 err;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);

  err = close(file);
  if (err != 0) return OkVal(IOError(vm));
  return OkVal(IntVal(Symbol("ok")));
}

Result VMRead(VM *vm)
{
  val size = VMStackPop(vm);
  val file = VMStackPop(vm);
  val result;
  u8 *data;
  i32 bytes_read;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = read(RawInt(file), data, RawInt(size));
  if (bytes_read >= 0) {
    MaybeGC(BinSpace(bytes_read) + 1, vm);
    result = NewBinary(bytes_read);
    Copy(data, BinaryData(result), bytes_read);
  } else {
    result = IOError(vm);
  }
  free(data);
  return OkVal(result);
}

Result VMWrite(VM *vm)
{
  val size = VMStackPop(vm);
  val buf = VMStackPop(vm);
  val file = VMStackPop(vm);
  i32 written;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsBinary(buf)) return RuntimeError("Data must be binary", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  written = write(RawInt(file), BinaryData(buf), RawInt(size));
  if (written < 0) return OkVal(IOError(vm));
  return OkVal(IntVal(written));
}

Result VMSeek(VM *vm)
{
  val whence = VMStackPop(vm);
  val offset = VMStackPop(vm);
  val file = VMStackPop(vm);
  i32 pos;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(offset)) return RuntimeError("Offset must be an integer", vm);
  if (!IsInt(whence)) return RuntimeError("Whence must be an integer", vm);

  pos = lseek(RawInt(file), RawInt(offset), RawInt(whence));
  if (pos < 0) return OkVal(IOError(vm));
  return OkVal(IntVal(pos));
}

static val IOError(VM *vm)
{
  val result;
  char *msg = strerror(errno);
  u32 len = strlen(msg);
  MaybeGC(BinSpace(len) + 4, vm);
  result = Tuple(2);
  TupleSet(result, 0, IntVal(Symbol("error")));
  TupleSet(result, 1, BinaryFrom(msg, len));
  return result;
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
  return OkVal(IntVal(RandomBetween(RawInt(min), RawInt(max))));
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
  {"panic!", VMPanic},
  {"typeof", VMTypeOf},
  {"byte", VMByte},
  {"symbol_name", VMSymbolName},
  {"time", VMTime},
  {"microtime", VMMicrotime},
  {"random", VMRandom},
  {"random_between", VMRandomBetween},
  {"seed", VMSeed},
  {"open", VMOpen},
  {"close", VMClose},
  {"read", VMRead},
  {"write", VMWrite},
  {"seek", VMSeek},
  {"sdl_new_window", SDLNewWindow},
  {"sdl_destroy_window", SDLDestroyWindow},
  {"sdl_present", SDLPresent},
  {"sdl_clear", SDLClear},
  {"sdl_line", SDLLine},
  {"sdl_set_color", SDLSetColor},
  {"sdl_poll_event", SDLPollEvent},
  {"sdl_get_ticks", SDLGetTicks},
  {"sdl_blit", SDLBlit}
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
