#include "primitives.h"
#include "mem.h"
#include "univ/math.h"
#include "univ/symbol.h"
#include "univ/time.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <termios.h>
#include <unistd.h>
#include "sdl.h"

static u32 IOError(char *msg, VM *vm);

u32 PrimitiveError(char *msg, VM *vm)
{
  vm->error = RuntimeError(msg, vm);
  return 0;
}

static u32 VMPanic(VM *vm)
{
  u32 a = StackPop();
  if (IsBinary(a)) {
    char *msg = BinToStr(a);
    vm->error = RuntimeError(msg, vm);
    free(msg);
  } else {
    vm->error = RuntimeError("Panic!", vm);
  }
  return a;
}

static u32 VMTypeOf(VM *vm)
{
  u32 a = StackPop();
  if (IsPair(a)) return IntVal(Symbol("pair"));
  if (IsTuple(a)) return IntVal(Symbol("tuple"));
  if (IsBinary(a)) return IntVal(Symbol("binary"));
  if (IsInt(a)) {
    char *name = SymbolName(RawVal(a));
    if (name) return IntVal(Symbol("symbol"));
    return IntVal(Symbol("integer"));
  }
  return IntVal(Symbol("unknown"));
}

static u32 VMByte(VM *vm)
{
  u32 a = StackPop();
  u32 num;
  u32 bin;
  if (!IsInt(a)) return PrimitiveError("Bad byte value", vm);
  num = RawInt(a);
  if (num < 0 || num > 255) return PrimitiveError("Bad byte value", vm);
  MaybeGC(2, vm);
  bin = NewBinary(1);
  BinaryData(bin)[0] = num;
  return bin;
}

static u32 VMSymbolName(VM *vm)
{
  u32 a = StackPop();
  char *name = SymbolName(RawVal(a));
  u32 bin;
  if (!name) return 0;
  MaybeGC(BinSpace(strlen(name)) + 1, vm);
  bin = BinaryFrom(name, strlen(name));
  return bin;
}

static u32 VMOpen(VM *vm)
{
  u32 flags = StackPop();
  u32 path = StackPop();
  char *str;
  i32 file;

  if (!IsBinary(path)) return PrimitiveError("Path must be a string", vm);
  if (!IsInt(flags)) return PrimitiveError("Flags must be an integer", vm);

  str = BinToStr(path);
  file = open(str, RawInt(flags), 0x1FF);
  free(str);
  if (file == -1) return IOError(strerror(errno), vm);
  return IntVal(file);
}

static u32 VMOpenSerial(VM *vm)
{
  u32 opts = StackPop();
  u32 speed = StackPop();
  u32 port = StackPop();
  i32 file;
  char *str;
  struct termios options;
  if (!IsBinary(port)) return PrimitiveError("Serial port must be a string", vm);
  if (!IsInt(speed)) return PrimitiveError("Speed must be an integer", vm);
  if (!IsInt(opts)) return PrimitiveError("Serial options must be an integer", vm);

  str = BinToStr(port);
  file = open(str, O_RDWR | O_NDELAY | O_NOCTTY);
  free(str);
  if (file == -1) return IOError(strerror(errno), vm);
  fcntl(file, F_SETFL, FNDELAY);

  tcgetattr(file, &options);
  cfsetispeed(&options, RawInt(speed));
  cfsetospeed(&options, RawInt(speed));
  options.c_cflag |= RawInt(opts);
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
  tcsetattr(file, TCSANOW, &options);

  return IntVal(file);
}

static u32 VMClose(VM *vm)
{
  u32 file = StackPop();
  u32 err;
  if (!IsInt(file)) return PrimitiveError("File must be an integer", vm);

  err = close(file);
  if (err != 0) return IOError(strerror(errno), vm);
  return IntVal(Symbol("ok"));
}

static u32 VMRead(VM *vm)
{
  u32 size = StackPop();
  u32 file = StackPop();
  u32 result;
  char *data;
  i32 bytes_read;

  if (!IsInt(file)) return PrimitiveError("File must be an integer", vm);
  if (!IsInt(size)) return PrimitiveError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = read(RawInt(file), data, RawInt(size));
  if (bytes_read < 0) return IOError(strerror(errno), vm);
  MaybeGC(BinSpace(bytes_read) + 1, vm);
  result = BinaryFrom(data, bytes_read);
  free(data);
  return result;
}

static u32 VMWrite(VM *vm)
{
  u32 buf = StackPop();
  u32 file = StackPop();
  i32 written;
  if (!IsInt(file)) return PrimitiveError("File must be an integer", vm);
  if (!IsBinary(buf)) return PrimitiveError("Data must be binary", vm);
  written = write(RawInt(file), BinaryData(buf), ObjLength(buf));
  if (written < 0) return IOError(strerror(errno), vm);
  return IntVal(written);
}

static u32 VMSeek(VM *vm)
{
  u32 whence = StackPop();
  u32 offset = StackPop();
  u32 file = StackPop();
  i32 pos;

  if (!IsInt(file)) return PrimitiveError("File must be an integer", vm);
  if (!IsInt(offset)) return PrimitiveError("Offset must be an integer", vm);
  if (!IsInt(whence)) return PrimitiveError("Whence must be an integer", vm);

  pos = lseek(RawInt(file), RawInt(offset), RawInt(whence));
  if (pos < 0) return IOError(strerror(errno), vm);
  return IntVal(pos);
}

static u32 VMListen(VM *vm)
{
  u32 portVal = StackPop();
  i32 status, s;
  char *port;
  struct addrinfo hints = {0};
  struct addrinfo *servinfo;

  if (!IsBinary(portVal)) return PrimitiveError("Port must be a string", vm);

  port = BinToStr(portVal);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  status = getaddrinfo(0, port, &hints, &servinfo);
  free(port);
  if (status != 0) return IOError((char*)gai_strerror(status), vm);

  s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s < 0) {
    freeaddrinfo(servinfo);
    return IOError(strerror(errno), vm);
  }

  status = bind(s, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);
  if (status < 0) return IOError(strerror(errno), vm);

  status = listen(s, 10);
  if (status < 0) return IOError(strerror(errno), vm);

  return IntVal(s);
}

static u32 VMAccept(VM *vm)
{
  u32 socketVal = StackPop();
  i32 s;
  if (!IsInt(socketVal)) return PrimitiveError("Socket must be an integer", vm);
  s = accept(RawInt(socketVal), 0, 0);
  if (s < 0) return IOError(strerror(errno), vm);
  return IntVal(s);
}

static u32 VMConnect(VM *vm)
{
  u32 portVal = StackPop();
  u32 nodeVal = StackPop();
  i32 status, s;
  char *node, *port;
  struct addrinfo hints = {0};
  struct addrinfo *servinfo;

  if (nodeVal != 0 && !IsBinary(nodeVal)) return PrimitiveError("Node must be a string", vm);
  if (!IsBinary(portVal)) return PrimitiveError("Port must be a string", vm);

  node = nodeVal ? BinToStr(nodeVal) : 0;
  port = BinToStr(portVal);

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(node, port, &hints, &servinfo);
  if (nodeVal) free(node);
  free(port);
  if (status != 0) return IOError((char*)gai_strerror(status), vm);

  s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s < 0) {
    freeaddrinfo(servinfo);
    return IOError(strerror(errno), vm);
  }
  status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
  freeaddrinfo(servinfo);

  if (status < 0) return IOError(strerror(errno), vm);

  return IntVal(s);
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

static u32 VMRandom(VM *vm)
{
  u32 r = RandomBetween(0, MaxIntVal);
  return IntVal(r);
}

static u32 VMRandomBetween(VM *vm)
{
  u32 max = StackPop();
  u32 min = StackPop();
  if (!IsInt(min) || !IsInt(max)) return PrimitiveError("Range must be integers", vm);
  return IntVal(RandomBetween(RawInt(min), RawInt(max)));
}

static u32 VMSeed(VM *vm)
{
  u32 seed = StackPop();
  if (!IsInt(seed)) return PrimitiveError("Seed must be an integer", vm);
  SeedRandom(RawInt(seed));
  return 0;
}

static u32 VMTime(VM *vm)
{
  return IntVal(Time());
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
#ifdef SDL
  {"sdl_new_window", SDLNewWindow},
  {"sdl_destroy_window", SDLDestroyWindow},
  {"sdl_present", SDLPresent},
  {"sdl_clear", SDLClear},
  {"sdl_line", SDLLine},
  {"sdl_set_color", SDLSetColor},
  {"sdl_poll_event", SDLPollEvent},
  {"sdl_get_ticks", SDLGetTicks},
  {"sdl_blit", SDLBlit}
#endif
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
