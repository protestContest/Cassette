#include "primitives.h"
#include "mem.h"
#include "sdl.h"
#include "univ/math.h"
#include "univ/symbol.h"
#include "univ/time.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>

static u32 IOError(char *msg, VM *vm);

static Result VMPanic(VM *vm)
{
  u32 a = StackPop();
  char *str = BinToStr(InspectVal(a));
  Result result = RuntimeError(str, vm);
  free(str);
  return result;
}

static Result VMTypeOf(VM *vm)
{
  u32 a = StackPop();
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

static Result VMByte(VM *vm)
{
  u32 a = StackPop();
  u32 num;
  u32 bin;
  if (!IsInt(a)) return RuntimeError("Bad byte value", vm);
  num = RawInt(a);
  if (num < 0 || num > 255) return RuntimeError("Bad byte value", vm);
  MaybeGC(2, vm);
  bin = NewBinary(1);
  BinaryData(bin)[0] = num;
  return OkVal(bin);
}

static Result VMSymbolName(VM *vm)
{
  u32 a = StackPop();
  char *name = SymbolName(RawVal(a));
  u32 bin;
  if (!name) return OkVal(0);
  MaybeGC(BinSpace(strlen(name)) + 1, vm);
  bin = BinaryFrom(name, strlen(name));
  return OkVal(bin);
}

static Result VMOpen(VM *vm)
{
  u32 flags = StackPop();
  u32 path = StackPop();
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

static Result VMOpenSerial(VM *vm)
{
  u32 opts = StackPop();
  u32 speed = StackPop();
  u32 port = StackPop();
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

static Result VMClose(VM *vm)
{
  u32 file = StackPop();
  u32 err;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);

  err = close(file);
  if (err != 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(Symbol("ok")));
}

static Result VMRead(VM *vm)
{
  u32 size = StackPop();
  u32 file = StackPop();
  u32 result;
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

static Result VMWrite(VM *vm)
{
  u32 buf = StackPop();
  u32 file = StackPop();
  i32 written;
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsBinary(buf)) return RuntimeError("Data must be binary", vm);
  written = write(RawInt(file), BinaryData(buf), ObjLength(buf));
  if (written < 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(written));
}

static Result VMSeek(VM *vm)
{
  u32 whence = StackPop();
  u32 offset = StackPop();
  u32 file = StackPop();
  i32 pos;

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(offset)) return RuntimeError("Offset must be an integer", vm);
  if (!IsInt(whence)) return RuntimeError("Whence must be an integer", vm);

  pos = lseek(RawInt(file), RawInt(offset), RawInt(whence));
  if (pos < 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(pos));
}

static Result VMListen(VM *vm)
{
  u32 portVal = StackPop();
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

static Result VMAccept(VM *vm)
{
  u32 socketVal = StackPop();
  i32 s;
  if (!IsInt(socketVal)) return RuntimeError("Socket must be an integer", vm);
  s = accept(RawInt(socketVal), 0, 0);
  if (s < 0) return OkVal(IOError(strerror(errno), vm));
  return OkVal(IntVal(s));
}

static Result VMConnect(VM *vm)
{
  u32 portVal = StackPop();
  u32 nodeVal = StackPop();
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

static u32 IOError(char *msg, VM *vm)
{
  u32 result;
  u32 len = strlen(msg);
  MaybeGC(BinSpace(len) + 4, vm);
  result = Tuple(2);
  TupleSet(result, 0, IntVal(Symbol("error")));
  TupleSet(result, 1, BinaryFrom(msg, len));
  return result;
}

static Result VMRandom(VM *vm)
{
  u32 r = RandomBetween(0, MaxIntVal);
  return OkVal(IntVal(r));
}

static Result VMRandomBetween(VM *vm)
{
  u32 max = StackPop();
  u32 min = StackPop();
  if (!IsInt(min) || !IsInt(max)) return RuntimeError("Range must be integers", vm);
  return OkVal(IntVal(RandomBetween(RawInt(min), RawInt(max))));
}

static Result VMSeed(VM *vm)
{
  u32 seed = StackPop();
  if (!IsInt(seed)) return RuntimeError("Seed must be an integer", vm);
  SeedRandom(RawInt(seed));
  return OkVal(0);
}

static Result VMTime(VM *vm)
{
  return OkVal(IntVal(Time()));
}

static PrimDef primitives[] = {
  {"panic!", VMPanic},
  {"typeof", VMTypeOf},
  {"byte", VMByte},
  {"symbol_name", VMSymbolName},
  {"time", VMTime},
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
