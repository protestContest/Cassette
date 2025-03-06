#include "runtime/primitives.h"
#include "runtime/mem.h"
#include "runtime/symbol.h"
#include "univ/file.h"
#include "univ/font.h"
#include "univ/math.h"
#include "univ/res.h"
#include "univ/str.h"
#include "univ/time.h"
#include "univ/vec.h"
#include "univ/window.h"

static u32 IOError(char *msg, VM *vm)
{
  StackPush(Tuple(2));
  TupleSet(StackPeek(0), 0, IntVal(Symbol("error")));
  TupleSet(StackPeek(0), 1, Binary(msg));
  return StackPop();
}

static u32 VMPanic(VM *vm)
{
  u32 a;
  assert(StackSize() >= 1);
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
  assert(StackSize() >= 1);
  a = StackPop();
  if (IsPair(a))    return IntVal(Symbol("pair"));
  if (IsTuple(a))   return IntVal(Symbol("tuple"));
  if (IsBinary(a))  return IntVal(Symbol("binary"));
  if (IsSymbol(a))  return IntVal(Symbol("symbol"));
  if (IsInt(a))     return IntVal(Symbol("integer"));
  return 0;
}

static u32 VMFormat(VM *vm)
{
  assert(StackSize() >= 1);
  return FormatVal(StackPop());
}

static u32 VMMakeTuple(VM *vm)
{
  u32 list, size = 0, tuple, i;
  assert(StackSize() >= 1);
  list = StackPeek(0);
  if (!IsPair(list)) return RuntimeError("Expected a list", vm);
  while (list) {
    size++;
    list = Tail(list);
  }

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
  assert(StackSize() >= 1);
  a = StackPop();
  name = SymbolName(RawVal(a));
  if (!name || !*name) return 0;
  bin = Binary(name);
  return bin;
}

static u32 VMOpen(VM *vm)
{
  u32 flags, path;
  char *str, *error;
  i32 file;
  assert(StackSize() >= 2);
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
  assert(StackSize() >= 3);
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
  assert(StackSize() >= 1);
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
  assert(StackSize() >= 2);
  size = StackPop();
  file = StackPop();

  if (!IsInt(file)) return RuntimeError("File must be an integer", vm);
  if (!IsInt(size)) return RuntimeError("Size must be an integer", vm);

  data = malloc(RawInt(size));
  bytes_read = Read(RawInt(file), data, RawInt(size), &error);
  if (error) return IOError(error, vm);
  result = BinaryFrom(data, bytes_read);
  free(data);
  return result;
}

static u32 VMWrite(VM *vm)
{
  u32 buf, file;
  i32 written;
  char *error;
  assert(StackSize() >= 2);
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
  assert(StackSize() >= 3);
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
  assert(StackSize() >= 1);
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
  assert(StackSize() >= 1);
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

  assert(StackSize() >= 2);

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
  assert(StackSize() >= 1);
  seed = StackPop();
  if (!IsInt(seed)) return RuntimeError("Seed must be an integer", vm);
  SeedRandom(RawInt(seed));
  return 0;
}

static u32 VMArgs(VM *vm)
{
  u32 i;
  u32 num_args = VecCount(vm->opts->program_args);

  StackPush(Tuple(num_args));
  for (i = 0; i < num_args; i++) {
    char *arg = vm->opts->program_args[i];
    TupleSet(StackPeek(0), i, Binary(arg));
  }
  return StackPop();
}

static u32 VMEnv(VM *vm)
{
  char *name;
  char *value;
  u32 nameVal, valueVal;

  assert(StackSize() >= 1);
  nameVal = StackPop();
  if (!IsBinary(nameVal)) return RuntimeError("Env variable must be a string", vm);
  name = StringFrom(BinaryData(nameVal), ObjLength(nameVal));
  value = getenv(name);
  if (value) {
    valueVal = Binary(value);
  } else {
    valueVal = 0;
  }
  free(name);
  return valueVal;
}

static u32 VMShell(VM *vm)
{
  char *cmd;
  u32 cmdVal;
  u32 result;

  assert(StackSize() >= 1);
  cmdVal = StackPeek(0);
  if (!IsBinary(cmdVal)) return RuntimeError("Command must be a string", vm);
  cmd = StringFrom(BinaryData(cmdVal), ObjLength(cmdVal));

  result = system(cmd);

  free(cmd);
  StackPop();
  return IntVal(result);
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
  assert(StackSize() >= 1);
  a = StackPop();
  if (!IsInt(a)) return RuntimeError("Only integers can be popcnt'd", vm);
  return IntVal(PopCount(RawInt(a)));
}

static u32 VMHash(VM *vm)
{
  assert(StackSize() >= 1);
  return HashVal(StackPop());
}

static u32 VMTime(VM *vm)
{
  return IntVal(Time());
}

static u32 VMMillis(VM *vm)
{
  return IntVal(Microtime()/1000);
}

static u32 VMNewWindow(VM *vm)
{
  /* new_window(title, width, height) */
  u32 title, width, height;
  CTWindow *w;
  u32 ref;

  assert(StackSize() >= 3);
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
  w = NewWindow(StringFrom(BinaryData(title), ObjLength(title)), RawInt(width), RawInt(height));
  ref = VMPushRef(w, vm);

  OpenWindow(w);
  NextEvent(0);

  return IntVal(ref);
}

static u32 VMDestroyWindow(VM *vm)
{
  CTWindow *w;
  assert(StackSize() >= 1);
  w = VMGetRef(RawVal(StackPop()), vm);
  if (!w) return RuntimeError("Invalid window reference", vm);
  CloseWindow(w);
  DestroyCanvas(&w->canvas);
  free(w->title);
  free(w);
  return 0;
}

static u32 VMUpdateWindow(VM *vm)
{
  CTWindow *w;
  assert(StackSize() >= 1);
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
  u32 where, msg;

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

  StackPush(Tuple(5));
  TupleSet(StackPeek(0), 0, EventTypeVal(event.what));
  TupleSet(StackPeek(0), 1, msg);
  TupleSet(StackPeek(0), 2, IntVal(event.when));
  where = Tuple(2);
  TupleSet(where, 0, IntVal(event.where.x));
  TupleSet(where, 1, IntVal(event.where.y));
  TupleSet(StackPeek(0), 3, where);
  TupleSet(StackPeek(0), 4, IntVal(event.modifiers));

  return StackPop();
}

static u32 VMWritePixel(VM *vm)
{
  /* write_pixel(x, y, color, window) */
  CTWindow *w;
  u32 x, y, color;
  assert(StackSize() >= 4);
  w = VMGetRef(RawVal(StackPop()), vm);
  color = StackPop();
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsBinary(color)) {
    return RuntimeError("Color must be a 32-bit RGBA binary", vm);
  }
  if (!IsInt(y)) return RuntimeError("Y must be an integer", vm);
  if (!IsInt(x)) return RuntimeError("X must be an integer", vm);
  color = *((u32*)BinaryData(color));
  PixelAt(&w->canvas, RawInt(x), RawInt(y)) = color;
  return 0;
}

static u32 VMMoveTo(VM *vm)
{
  CTWindow *w;
  u32 x, y;
  assert(StackSize() >= 3);
  w = VMGetRef(RawVal(StackPop()), vm);
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(x)) return RuntimeError("Coordinates must be integers", vm);
  if (!IsInt(y)) return RuntimeError("Coordinates must be integers", vm);

  MoveTo(RawInt(x), RawInt(y), &w->canvas);

  return 0;
}

static u32 VMMove(VM *vm)
{
  CTWindow *w;
  u32 x, y;
  assert(StackSize() >= 3);
  w = VMGetRef(RawVal(StackPop()), vm);
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(x)) return RuntimeError("Coordinates must be integers", vm);
  if (!IsInt(y)) return RuntimeError("Coordinates must be integers", vm);

  Move(RawInt(x), RawInt(y), &w->canvas);

  return 0;
}

static u32 VMSetColor(VM *vm)
{
  CTWindow *w;
  u32 color;
  assert(StackSize() >= 2);
  w = VMGetRef(RawVal(StackPop()), vm);
  color = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsBinary(color)) return RuntimeError("Color must be a 32-bit RGBA binary", vm);

  w->canvas.pen.color = *((u32*)BinaryData(color));
  printf("%08X\n", w->canvas.pen.color);

  return 0;
}

static u32 VMSetFont(VM *vm)
{
  CTWindow *w;
  u32 name, size;
  char *name_str;
  assert(StackSize() >= 3);
  w = VMGetRef(RawVal(StackPop()), vm);
  size = StackPop();
  name = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsBinary(name)) return RuntimeError("Font name must be a string", vm);
  if (!IsInt(size)) return RuntimeError("Font size must be an integer", vm);

  name_str = BinToStr(name);
  SetFont(name_str, RawInt(size), &w->canvas);
  free(name_str);
  return 0;
}

static u32 VMDrawString(VM *vm)
{
  CTWindow *w;
  u32 text;
  char *str;
  assert(StackSize() >= 2);
  w = VMGetRef(RawVal(StackPop()), vm);
  text = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsBinary(text)) return RuntimeError("Text must be a string", vm);

  str = BinToStr(text);
  DrawString(str, &w->canvas);
  free(str);
  return 0;
}

static u32 VMStringWidth(VM *vm)
{
  CTWindow *w;
  u32 text, width;
  char *str;
  assert(StackSize() >= 2);
  w = VMGetRef(RawVal(StackPop()), vm);
  text = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsBinary(text)) return RuntimeError("Text must be a string", vm);

  str = BinToStr(text);
  width = IntVal(StringWidth(str, &w->canvas));
  free(str);
  return width;
}

static u32 VMLineTo(VM *vm)
{
  CTWindow *w;
  u32 x, y;
  assert(StackSize() >= 3);
  w = VMGetRef(RawVal(StackPop()), vm);
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(x)) return RuntimeError("Coordinates must be integers", vm);
  if (!IsInt(y)) return RuntimeError("Coordinates must be integers", vm);

  LineTo(RawInt(x), RawInt(y), &w->canvas);

  return 0;
}

static u32 VMLine(VM *vm)
{
  CTWindow *w;
  u32 x, y;
  assert(StackSize() >= 3);
  w = VMGetRef(RawVal(StackPop()), vm);
  y = StackPop();
  x = StackPop();
  if (!w) return RuntimeError("Invalid window reference", vm);
  if (!IsInt(x)) return RuntimeError("Coordinates must be integers", vm);
  if (!IsInt(y)) return RuntimeError("Coordinates must be integers", vm);

  Line(RawInt(x), RawInt(y), &w->canvas);

  return 0;
}

static u32 VMFillRect(VM *vm)
{
  CTWindow *w;
  u32 x0, y0, x1, y1, color;
  BBox r;

  assert(StackSize() >= 5);
  w = VMGetRef(RawVal(StackPop()), vm);
  color = StackPop();
  y1 = StackPop();
  x1 = StackPop();
  y0 = StackPop();
  x0 = StackPop();
  if (!IsInt(x0) || !IsInt(y0) || !IsInt(x1) || !IsInt(y1)) {
    return RuntimeError("Coordinates must be integers", vm);
  }
  if (!IsBinary(color) || ObjLength(color) != 4) {
    return RuntimeError("Color must be a 32-bit RGBA binary", vm);
  }
  color = *((u32*)BinaryData(color));

  r.left = RawInt(x0);
  r.top = RawInt(y0);
  r.right = RawInt(x1);
  r.bottom = RawInt(y1);
  FillRect(&r, color, &w->canvas);

  return 0;
}

static u32 VMBlit(VM *vm)
{
  /* blit(data, width, height, x, y, window) */
  CTWindow *w;
  i32 data, width, height, x, y;
  u32 *pixels;
  assert(StackSize() >= 6);
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
  height = RawInt(height);
  x = RawInt(x);
  y = RawInt(y);
  pixels = (u32*)BinaryData(data);
  Blit(pixels, width, height, x, y, &w->canvas);
  return 0;
}

static u32 VMUseResources(VM *vm)
{
  u32 res;
  char *filename;
  assert(StackSize() >= 1);
  res = StackPop();
  if (!IsBinary(res)) return RuntimeError("Filename must be a string", vm);
  filename = BinToStr(res);
  OpenResFile(filename);
  free(filename);
  return 0;
}

static u32 VMGetPen(VM *vm)
{
  CTWindow *w;
  u32 canvas;
  assert(StackSize() >= 1);
  w = VMGetRef(RawVal(StackPeek(0)), vm);
  if (!w) return RuntimeError("Invalid window reference", vm);
  canvas = Tuple(2);
  TupleSet(canvas, 0, IntVal(w->canvas.pen.x));
  TupleSet(canvas, 1, IntVal(w->canvas.pen.y));
  StackPop();
  return canvas;
}

static u32 VMGetFont(VM *vm)
{
  CTWindow *w;
  u32 font_info;
  FontInfo info;
  assert(StackSize() >= 1);
  w = VMGetRef(RawVal(StackPeek(0)), vm);
  if (!w) return RuntimeError("Invalid window reference", vm);
  GetFontInfo(&info, &w->canvas);
  font_info = Tuple(4);
  TupleSet(font_info, 0, IntVal(info.ascent));
  TupleSet(font_info, 1, IntVal(info.descent));
  TupleSet(font_info, 2, IntVal(info.widMax));
  TupleSet(font_info, 3, IntVal(info.leading));
  StackPop();
  return font_info;
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
  /* System */
  {"time", VMTime},
  {"millis", VMMillis},
  {"random", VMRandom},
  {"seed", VMSeed},
  {"args", VMArgs},
  {"env", VMEnv},
  {"shell", VMShell},
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
  {"move_to", VMMoveTo},
  {"move", VMMove},
  {"set_color", VMSetColor},
  {"set_font", VMSetFont},
  {"draw_string", VMDrawString},
  {"string_width", VMStringWidth},
  {"line_to", VMLineTo},
  {"line", VMLine},
  {"fill_rect", VMFillRect},
  {"blit", VMBlit},
  {"use_resources", VMUseResources},
  {"get_pen", VMGetPen},
  {"font_info", VMGetFont}
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

