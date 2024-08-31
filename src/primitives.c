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
#include <termios.h>
#include <netdb.h>

static val IOError(char *msg, VM *vm);

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
  if (file == -1) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(file));
}

Result VMOpenSerial(VM *vm)
{
  val opts = VMStackPop(vm);
  val speed = VMStackPop(vm);
  val port = VMStackPop(vm);
  i32 file;
  char *str;
  struct termios options;
  if (!IsBinary(port)) return RuntimeError("Serial port must be a string", vm);
  if (!IsInt(speed)) return RuntimeError("Speed must be an integer", vm);
  if (!IsInt(opts)) return RuntimeError("Serial options must be an integer", vm);

  str = BinToStr(port);
  file = open(str, O_RDWR | O_NDELAY | O_NOCTTY);
  free(str);
  if (file == -1) return OkVal(IOError(strerror(errno), vm));
  fcntl(file, F_SETFL, FNDELAY);

  tcgetattr(file, &options);
  cfsetispeed(&options, RawInt(speed));
  cfsetospeed(&options, RawInt(speed));
  options.c_cflag |= RawInt(opts);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
  tcsetattr(file, TCSANOW, &options);

  return OkVal(IntVal(file));
}

Result VMClose(VM *vm)
{
  val file = VMStackPop(vm);
  u32 err;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);

  err = close(file);
  if (err != 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(Symbol("ok")));
}

Result VMRead(VM *vm)
{
  val size = VMStackPop(vm);
  val file = VMStackPop(vm);
  val result;
  char *data;
  i32 bytes_read;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = read(RawInt(file), data, RawInt(size));
  if (bytes_read < 0) return OkVal(IOError(strerror(errno), vm));
  MaybeGC(BinSpace(bytes_read) + 1, vm);
  result = BinaryFrom(data, bytes_read);
  free(data);
  return OkVal(result);
}

Result VMWrite(VM *vm)
{
  val buf = VMStackPop(vm);
  val file = VMStackPop(vm);
  i32 written;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsBinary(buf)) return RuntimeError("Data must be binary", vm);
  written = write(RawInt(file), BinaryData(buf), BinaryLength(buf));
  if (written < 0) return OkVal(IOError(strerror(errno), vm));
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
  if (pos < 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(pos));
}

Result VMListen(VM *vm)
{
  val portVal = VMStackPop(vm);
  i32 status, s;
  char *port;
  struct addrinfo hints = {0};
  struct addrinfo *servinfo;

  if (!IsBinary(portVal)) return RuntimeError("Port must be a string", vm);

  port = BinToStr(portVal);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  status = getaddrinfo(0, port, &hints, &servinfo);
  free(port);
  if (status != 0) return OkVal(IOError((char*)gai_strerror(status), vm));

  s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s < 0) {
    freeaddrinfo(servinfo);
    return OkVal(IOError(strerror(errno), vm));
  }

  status = bind(s, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);
  if (status < 0) return OkVal(IOError(strerror(errno), vm));

  status = listen(s, 10);
  if (status < 0) return OkVal(IOError(strerror(errno), vm));

  return OkVal(IntVal(s));
}

Result VMAccept(VM *vm)
{
  val socketVal = VMStackPop(vm);
  i32 s;
  if (!IsInt(socketVal)) return RuntimeError("Socket must be an integer", vm);
  s = accept(RawInt(socketVal), 0, 0);
  if (s < 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(s));
}

Result VMConnect(VM *vm)
{
  val portVal = VMStackPop(vm);
  val nodeVal = VMStackPop(vm);
  i32 status, s;
  char *node, *port;
  struct addrinfo hints = {0};
  struct addrinfo *servinfo;

  if (nodeVal != 0 && !IsBinary(nodeVal)) return RuntimeError("Node must be a string", vm);
  if (!IsBinary(portVal)) return RuntimeError("Port must be a string", vm);

  node = nodeVal ? BinToStr(nodeVal) : 0;
  port = BinToStr(portVal);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(node, port, &hints, &servinfo);
  if (nodeVal) free(node);
  free(port);
  if (status != 0) return OkVal(IOError((char*)gai_strerror(status), vm));

  s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s < 0) {
    freeaddrinfo(servinfo);
    return OkVal(IOError(strerror(errno), vm));
  }
  status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);

  if (status < 0) return OkVal(IOError(strerror(errno), vm));

  return OkVal(IntVal(s));
}

static val IOError(char *msg, VM *vm)
{
  val result;
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
  {"open_serial", VMOpenSerial},
  {"close", VMClose},
  {"read", VMRead},
  {"write", VMWrite},
  {"seek", VMSeek},
  {"listen", VMListen},
  {"accept", VMAccept},
  {"connect", VMConnect},
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
