#include "runtime/primitives.h"
#include "runtime/mem.h"
#include "runtime/symbol.h"
#include "univ/file.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/time.h"
#include "univ/window.h"

static u32 IOError(char *msg, VM *vm)
{
  u32 result;
  u32 len = StrLen(msg);
  MaybeGC(BinSpace(len) + 5, vm);
  result = Tuple(2);
  TupleSet(result, 0, IntVal(Symbol("error")));
  TupleSet(result, 1, BinaryFrom(msg, len));
  return result;
}

static u32 VMPanic(VM *vm)
{
  u32 a;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  a = StackPop();
  if (IsBinary(a)) {
    char *msg = BinToStr(a);
    RuntimeError(msg, vm);
    free(msg);
  } else {
    RuntimeError("Panic!", vm);
  }
  return a;
}

static u32 VMTypeOf(VM *vm)
{
  u32 a;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  a = StackPop();
  if (IsPair(a))    return IntVal(Symbol("pair"));
  if (IsTuple(a))   return IntVal(Symbol("tuple"));
  if (IsBinary(a))  return IntVal(Symbol("binary"));
  if (IsSymbol(a))  return IntVal(Symbol("symbol"));
  if (IsInt(a))     return IntVal(Symbol("integer"));
  return 0;
}

static u32 FormatSize(u32 value)
{
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) < 256) return 1;
  if (IsBinary(value)) return ObjLength(value);
  if (value && IsPair(value)) {
    return FormatSize(Head(value)) + FormatSize(Tail(value));
  }
  return 0;
}

static u8 *FormatVal(u32 value, u8 *buf)
{
  if (IsInt(value) && RawInt(value) >= 0 && RawInt(value) < 256) {
    *buf = (u8)RawInt(value);
    return buf + 1;
  }
  if (IsBinary(value)) {
    Copy(BinaryData(value), buf, ObjLength(value));
    return buf + ObjLength(value);
  }
  if (value && IsPair(value)) {
    buf = FormatVal(Head(value), buf);
    buf = FormatVal(Tail(value), buf);
    return buf;
  }
  return buf;
}

static u32 VMFormat(VM *vm)
{
  u32 a, size, bin;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  size = FormatSize(StackPeek(0));
  MaybeGC(BinSpace(size) + 1, vm);
  bin = NewBinary(size);
  a = StackPop();
  FormatVal(a, (u8*)BinaryData(bin));
  return bin;
}

static u32 VMMakeTuple(VM *vm)
{
  u32 list, size = 0, tuple, i;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  list = StackPeek(0);
  if (!IsPair(list)) return RuntimeError("Expected a list", vm);
  while (list) {
    size++;
    list = Tail(list);
  }
  MaybeGC(size+1, vm);
  tuple = Tuple(size);
  list = StackPop();
  for (i = 0; i < size; i++) {
    TupleSet(tuple, i, Head(list));
    list = Tail(list);
  }
  return tuple;
}

static u32 VMSymbolName(VM *vm)
{
  u32 a;
  char *name;
  u32 bin;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  a = StackPop();
  name = SymbolName(RawVal(a));
  if (!name || !*name) return 0;
  MaybeGC(BinSpace(StrLen(name)) + 2, vm);
  bin = BinaryFrom(name, StrLen(name));
  return bin;
}

static u32 VMOpen(VM *vm)
{
  u32 flags, path;
  char *str, *error;
  i32 file;
  if (StackSize() < 2) return RuntimeError("Stack underflow", vm);
  flags = StackPop();
  path = StackPop();

  if (!IsBinary(path)) return RuntimeError("Path must be a string", vm);
  if (!IsInt(flags)) return RuntimeError("Flags must be an integer", vm);

  str = BinToStr(path);
  file = Open(str, RawInt(flags), &error);
  free(str);
  if (error) return IOError(error, vm);
  return IntVal(file);
}

static u32 VMOpenSerial(VM *vm)
{
  u32 opts, port, speed;
  i32 file;
  char *str, *error;
  if (StackSize() < 3) return RuntimeError("Stack underflow", vm);
  opts = StackPop();
  speed = StackPop();
  port = StackPop();
  if (!IsBinary(port)) return RuntimeError("Serial port must be a string", vm);
  if (!IsInt(speed)) return RuntimeError("Speed must be an integer", vm);
  if (!IsInt(opts)) {
    return RuntimeError("Serial options must be an integer", vm);
  }

  str = BinToStr(port);
  file = OpenSerial(str, RawInt(speed), RawInt(opts), &error);
  if (error) return IOError(error, vm);
  free(str);

  return IntVal(file);
}

static u32 VMClose(VM *vm)
{
  u32 file;
  char *err;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  file = StackPop();
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);

  err = Close(file);
  if (err) return IOError(err, vm);
  return IntVal(Symbol("ok"));
}

static u32 VMRead(VM *vm)
{
  u32 result, size, file;
  char *data, *error;
  i32 bytes_read;
  if (StackSize() < 2) return RuntimeError("Stack underflow", vm);
  size = StackPop();
  file = StackPop();

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = Read(RawInt(file), data, RawInt(size), &error);
  if (error) return IOError(error, vm);
  MaybeGC(BinSpace(bytes_read) + 2, vm);
  result = BinaryFrom(data, bytes_read);
  free(data);
  return result;
}

static u32 VMWrite(VM *vm)
{
  u32 buf, file;
  i32 written;
  char *error;
  if (StackSize() < 2) return RuntimeError("Stack underflow", vm);
  buf = StackPop();
  file = StackPop();
  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsBinary(buf)) return RuntimeError("Data must be binary", vm);
  written = Write(RawInt(file), BinaryData(buf), ObjLength(buf), &error);
  if (error) return IOError(error, vm);
  return IntVal(written);
}

static u32 VMSeek(VM *vm)
{
  u32 whence, offset, file;
  i32 pos;
  char *error;
  if (StackSize() < 3) return RuntimeError("Stack underflow", vm);
  whence = StackPop();
  offset = StackPop();
  file = StackPop();

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(offset)) return RuntimeError("Offset must be an integer", vm);
  if (!IsInt(whence)) return RuntimeError("Whence must be an integer", vm);

  pos = Seek(RawInt(file), RawInt(offset), RawInt(whence), &error);
  if (error) return IOError(error, vm);
  return IntVal(pos);
}

static u32 VMListen(VM *vm)
{
  u32 portVal;
  i32 s;
  char *port, *error;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  portVal = StackPop();
  if (!IsBinary(portVal)) return RuntimeError("Port must be a string", vm);

  port = BinToStr(portVal);
  s = Listen(port, &error);
  free(port);
  if (error) return IOError(error, vm);
  return IntVal(s);
}

static u32 VMAccept(VM *vm)
{
  u32 socketVal;
  i32 s;
  char *error;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  socketVal = StackPop();
  if (!IsInt(socketVal)) return RuntimeError("Socket must be an integer", vm);
  s = Accept(RawInt(socketVal), &error);
  if (error) return IOError(error, vm);
  return IntVal(s);
}

static u32 VMConnect(VM *vm)
{
  u32 portVal, nodeVal;
  i32 s;
  char *node, *port, *error;

  if (StackSize() < 2) return RuntimeError("Stack underflow", vm);

  portVal = StackPop();
  nodeVal = StackPop();
  if (nodeVal != 0 && !IsBinary(nodeVal)) {
    return RuntimeError("Node must be a string", vm);
  }
  if (!IsBinary(portVal)) return RuntimeError("Port must be a string", vm);

  node = nodeVal ? BinToStr(nodeVal) : 0;
  port = BinToStr(portVal);

  s = Connect(node, port, &error);
  if (node) free(node);
  free(port);
  if (error) return IOError(error, vm);

  return IntVal(s);
}

static u32 VMRandom(VM *vm)
{
  return IntVal(Random());
}

static u32 VMSeed(VM *vm)
{
  u32 seed;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  seed = StackPop();
  if (!IsInt(seed)) return RuntimeError("Seed must be an integer", vm);
  SeedRandom(RawInt(seed));
  return 0;
}

static u32 VMMaxInt(VM *vm)
{
  return MaxIntVal;
}

static u32 VMMinInt(VM *vm)
{
  return MinIntVal;
}

static u32 VMPopCount(VM *vm)
{
  i32 a;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  a = StackPop();
  if (!IsInt(a)) return RuntimeError("Only integers can be popcnt'd", vm);
  return IntVal(PopCount(RawInt(a)));
}

static u32 VMHash(VM *vm)
{
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  return HashVal(StackPop());
}

static u32 VMTime(VM *vm)
{
  return IntVal(Time());
}

static u32 VMNewWindow(VM *vm)
{
  /* new_window(title, width, height) */
  u32 title, width, height;
  CTWindow *w;
  u32 ref;

  if (StackSize() < 3) return RuntimeError("Stack underflow", vm);
  height = StackPop();
  width = StackPop();
  title = StackPop();
  if (!IsInt(width) || !IsInt(height)) {
    return RuntimeError("Width and height must be integers", vm);
  }
  if (!IsBinary(title)) {
    return RuntimeError("Title must be a string", vm);
  }

  w = malloc(sizeof(CTWindow));
  w->title = StringFrom(BinaryData(title), ObjLength(title));
  w->width = RawInt(width);
  w->height = RawInt(height);
  w->buf = calloc(1, sizeof(u32)*w->width*w->height);
  ref = VMPushRef(w, vm);

  OpenWindow(w);
  NextEvent(0);

  return IntVal(ref);
}

static u32 VMDestroyWindow(VM *vm)
{
  CTWindow *w;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  w = VMGetRef(RawVal(StackPop()), vm);
  if (!w) return RuntimeError("Invalid window reference", vm);
  CloseWindow(w);
  free(w->buf);
  free(w->title);
  free(w);
  return 0;
}

static u32 VMUpdateWindow(VM *vm)
{
  CTWindow *w;
  if (StackSize() < 1) return RuntimeError("Stack underflow", vm);
  w = VMGetRef(RawVal(StackPop()), vm);
  if (!w) return RuntimeError("Invalid window reference", vm);
  UpdateWindow(w);
  return 0;
}

static u32 EventTypeVal(u32 id)
{
  switch (id) {
  case mouseDown: return IntVal(Symbol("mouseDown"));
  case mouseUp:   return IntVal(Symbol("mouseUp"));
  case keyDown:   return IntVal(Symbol("keyDown"));
  case keyUp:     return IntVal(Symbol("keyUp"));
  case autoKey:   return IntVal(Symbol("autoKey"));
  case quitEvent: return IntVal(Symbol("quit"));
  default:        return 0;
  }
}

static u32 VMPollEvent(VM *vm)
{
  Event event;
  u32 eventVal, where, msg;

  NextEvent(&event);

  msg = 0;
  switch (event.what) {
  case keyDown:
  case keyUp:
  case autoKey:
    msg = IntVal(event.message.key.code << 8 | event.message.key.c);
    break;
  case mouseDown:
  case mouseUp:
    msg = IntVal(VMFindRef(event.message.window, vm));
    break;
  }

  MaybeGC(9, vm);
  eventVal = Tuple(5);
  TupleSet(eventVal, 0, EventTypeVal(event.what));
  TupleSet(eventVal, 1, msg);
  TupleSet(eventVal, 2, IntVal(event.when));
  where = Tuple(2);
  TupleSet(where, 0, IntVal(event.where.x));
  TupleSet(where, 1, IntVal(event.where.y));
  TupleSet(eventVal, 3, where);
  TupleSet(eventVal, 4, IntVal(event.modifiers));

  return eventVal;
}

static u32 VMWritePixel(VM *vm)
{
  /* write_pixel(x, y, color, window) */
  CTWindow *w;
  u32 x, y, color;
  if (StackSize() < 4) return RuntimeError("Stack underflow", vm);
  w = VMGetRef(RawVal(StackPop()), vm);
  color = StackPop();
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(color)) return RuntimeError("Color must be an integer", vm);
  if (!IsInt(y)) return RuntimeError("Y must be an integer", vm);
  if (!IsInt(x)) return RuntimeError("X must be an integer", vm);
  WritePixel(w, RawInt(x), RawInt(y)) = RawInt(color);
  return 0;
}

static u32 VMBlit(VM *vm)
{
  /* blit(data, width, height, x, y, window) */
  CTWindow *w;
  i32 data, width, height, x, y, sx, sy, i, rowWidth;
  u32 *pixels;
  if (StackSize() < 6) return RuntimeError("Stack underflow", vm);
  w = VMGetRef(RawVal(StackPop()), vm);
  y = StackPop();
  x = StackPop();
  height = StackPop();
  width = StackPop();
  data = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(y)) return RuntimeError("Y must be an integer", vm);
  if (!IsInt(x)) return RuntimeError("X must be an integer", vm);
  if (!IsInt(height)) return RuntimeError("Height must be an integer", vm);
  if (!IsInt(width)) return RuntimeError("Width must be an integer", vm);
  if (!IsBinary(data)) return RuntimeError("Data must be binary", vm);

  width = RawInt(width);
  rowWidth = width;
  height = RawInt(height);
  x = RawInt(x);
  y = RawInt(y);
  sx = 0;
  sy = 0;
  pixels = (u32*)BinaryData(data);

  /* dst is outside window */
  if (x >= w->width) return 0;
  if (y >= w->height) return 0;

  /* clip src to fit in window */
  if (x < 0) {
    width += x;
    sx = -x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    sy = -y;
    y = 0;
  }
  if (x + width > w->width) width = w->width - x;
  if (y + height > w->height) height = w->height - y;

  /* nothing to copy */
  if (width <= 0 || height <= 0) return 0;

  /* check source data size */
  if ((sx+width)*(sy+height) >= (i32)ObjLength(data)) {
    return RuntimeError("Source data too small", vm);
  }

  /* copy each row */
  for (i = 0; i < height; i++) {
    u32 *src = pixels + (sy+i)*rowWidth + sx;
    u32 *dst = w->buf + (y+i)*w->width + x;
    Copy(src, dst, width*sizeof(u32));
  }

  return 0;
}

static PrimDef primitives[] = {
  {"panic!", VMPanic},
  {"typeof", VMTypeOf},
  {"format", VMFormat},
  {"make_tuple", VMMakeTuple},
  {"symbol_name", VMSymbolName},
  {"hash", VMHash},
  {"popcount", VMPopCount},
  {"max_int", VMMaxInt},
  {"min_int", VMMinInt},
  {"time", VMTime},
  {"random", VMRandom},
  {"seed", VMSeed},
  /* I/O */
  {"open", VMOpen},
  {"open_serial", VMOpenSerial},
  {"close", VMClose},
  {"read", VMRead},
  {"write", VMWrite},
  {"seek", VMSeek},
  {"listen", VMListen},
  {"accept", VMAccept},
  {"connect", VMConnect},
  /* Graphics */
  {"open_window", VMNewWindow},
  {"close_window", VMDestroyWindow},
  {"update_window", VMUpdateWindow},
  {"next_event", VMPollEvent},
  {"write_pixel", VMWritePixel},
  {"blit", VMBlit}
};

i32 PrimitiveID(u32 name)
{
  u32 i;
  for (i = 0; i < ArrayCount(primitives); i++) {
    if (Symbol(primitives[i].name) == name) return i;
  }
  return -1;
}

char *PrimitiveName(u32 id)
{
  return primitives[id].name;
}

PrimFn PrimitiveFn(u32 id)
{
  return primitives[id].fn;
}

