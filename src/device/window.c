#include "window.h"
#include "canvas/canvas.h"
#include "univ/str.h"
#include "mem/symbols.h"
#include "univ/system.h"

static Val MakeColor(u32 color, Mem *mem);
static u32 ColorFrom(Val c, Mem *mem);
static bool IsColor(Val color, Mem *mem);

Result WindowOpen(Val opts, Mem *mem)
{
  Val width, height;
  char *title;
  Result result = OkResult(Nil);

  if (!IsTuple(opts, mem)) {
    return ErrorResult("Opts must be a tuple", 0, 0);
  }

  if (TupleLength(opts, mem) == 2) {
    title = "Cassette";
    width = TupleGet(opts, 0, mem);
    height = TupleGet(opts, 1, mem);
  } else {
    Val title_val = TupleGet(opts, 0, mem);
    if (!IsBinary(title_val, mem)) return ErrorResult("Expected string", 0, 0);
    title = CopyStr(BinaryData(title_val, mem), BinaryLength(title_val, mem));
    width = TupleGet(opts, 1, mem);
    height = TupleGet(opts, 2, mem);
  }
  if (!IsInt(width)) return ErrorResult("Expected integer", 0, 0);
  if (!IsInt(height)) return ErrorResult("Expected integer", 0, 0);

  result.data = MakeCanvas(RawInt(width), RawInt(height), title);
  return result;
}

Result WindowClose(void *context, Mem *mem)
{
  FreeCanvas((Canvas*)context);
  return OkResult(Ok);
}

Result WindowRead(void *context, Val length, Mem *mem)
{
  /* to do: return input events */
  return ErrorResult("Unimplemented", 0, 0);
}

Result WindowWrite(void *context, Val cmd, Mem *mem)
{
  Val type;
  if (!IsTuple(cmd, mem)) return ErrorResult("Invalid window command", 0, 0);
  if (TupleLength(cmd, mem) == 0) return ErrorResult("Invalid window command", 0, 0);
  type = TupleGet(cmd, 0, mem);
  if (!IsSym(type)) return ErrorResult("Invalid window command", 0, 0);

  if (type == SymbolFor("clear")) {
    Val color;
    if (TupleLength(cmd, mem) != 2) return ErrorResult("Invalid window command", 0, 0);
    color = TupleGet(cmd, 1, mem);
    if (!IsColor(color, mem)) return ErrorResult("Invalid color", 0, 0);

    ClearCanvas((Canvas*)context, ColorFrom(color, mem));
    return OkResult(Nil);
  } else if (type == SymbolFor("text")) {
    Val string, x, y;
    char *text;
    if (TupleLength(cmd, mem) != 4) return ErrorResult("Invalid window command", 0, 0);
    string = TupleGet(cmd, 1, mem);
    x = TupleGet(cmd, 2, mem);
    y = TupleGet(cmd, 3, mem);
    if (!IsBinary(string, mem)) return ErrorResult("Invalid window command", 0, 0);
    if (!IsNum(x, mem)) return ErrorResult("Invalid window command", 0, 0);
    if (!IsNum(y, mem)) return ErrorResult("Invalid window command", 0, 0);

    text = CopyStr(BinaryData(string, mem), BinaryLength(string, mem));
    DrawText(text, RawNum(x), RawNum(y), (Canvas*)context);
    Free(text);

    return OkResult(Nil);
  } else if (type == SymbolFor("line")) {
    Val x1, y1, x2, y2;
    if (TupleLength(cmd, mem) != 5) return ErrorResult("Invalid window command", 0, 0);
    x1 = TupleGet(cmd, 1, mem);
    y1 = TupleGet(cmd, 2, mem);
    x2 = TupleGet(cmd, 3, mem);
    y2 = TupleGet(cmd, 4, mem);
    if (!IsNum(x1, mem)) return ErrorResult("Invalid window command", 0, 0);
    if (!IsNum(y1, mem)) return ErrorResult("Invalid window command", 0, 0);
    if (!IsNum(x2, mem)) return ErrorResult("Invalid window command", 0, 0);
    if (!IsNum(y2, mem)) return ErrorResult("Invalid window command", 0, 0);

    DrawLine(RawNum(x1), RawNum(y1), RawNum(x2), RawNum(y2), (Canvas*)context);

    return OkResult(Nil);
  } else {
    return ErrorResult("Invalid window command", 0, 0);
  }
}

Result WindowSet(void *context, Val key, Val value, Mem *mem)
{
  Canvas *canvas = (Canvas*)context;
  if (key == SymbolFor("width")) {
    return ErrorResult("Read only", 0, 0);
  } else if (key == SymbolFor("height")) {
    return ErrorResult("Read only", 0, 0);
  } else if (key == SymbolFor("font")) {
    char *filename;
    u32 size;
    if (IsTuple(value, mem) && TupleLength(value, mem) == 2) {
      Val bin = TupleGet(value, 0, mem);
      if (!IsBinary(bin, mem)) return ErrorResult("Expected {string, integer}", 0, 0);
      if (!IsInt(TupleGet(value, 1, mem))) return ErrorResult("Expected {string, integer}", 0, 0);
      filename = CopyStr(BinaryData(bin, mem), BinaryLength(bin, mem));
      size = RawInt(TupleGet(value, 1, mem));
    } else if (IsBinary(value, mem)) {
      filename = CopyStr(BinaryData(value, mem), BinaryLength(value, mem));
      size = canvas->font_size;
    } else {
      return ErrorResult("Expected string or {string, integer}", 0, 0);
    }

    if (canvas->font_size != size || !StrEq(canvas->font_filename, filename)) {
      SetFont(canvas, filename, size);
    }
    Free(filename);

    return OkResult(Ok);
  } else if (key == SymbolFor("font-size")) {
    u32 size = RawInt(value);
    if (!IsInt(value)) return ErrorResult("Expected integer", 0, 0);
    if (canvas->font_size != size) {
      canvas->font_size = size;
      SetFont(canvas, canvas->font_filename, size);
    }
    return OkResult(Ok);
  } else if (key == SymbolFor("color")) {
    if (!IsColor(value, mem)) return ErrorResult("Invalid color", 0, 0);
    canvas->color = ColorFrom(value, mem);
    return OkResult(Ok);
  } else {
    return OkResult(Nil);
  }
}

Result WindowGet(void *context, Val key, Mem *mem)
{
  Canvas *canvas = (Canvas*)context;
  if (key == SymbolFor("width")) {
    return OkResult(IntVal(canvas->width));
  } else if (key == SymbolFor("height")) {
    return OkResult(IntVal(canvas->height));
  } else if (key == SymbolFor("font")) {
    Val font = BinaryFrom(canvas->font_filename, StrLen(canvas->font_filename), mem);
    return OkResult(font);
  } else if (key == SymbolFor("font-size")) {
    return OkResult(IntVal(canvas->font_size));
  } else if (key == SymbolFor("color")) {
    return OkResult(MakeColor(canvas->color, mem));
  } else {
    return OkResult(Nil);
  }
}

static Val MakeColor(u32 color, Mem *mem)
{
  Val c = MakeTuple(3, mem);
  TupleSet(c, 0, IntVal(Red(color)), mem);
  TupleSet(c, 1, IntVal(Green(color)), mem);
  TupleSet(c, 2, IntVal(Blue(color)), mem);
  return c;
}

static u32 ColorFrom(Val c, Mem *mem)
{
  u32 red = RawInt(TupleGet(c, 0, mem));
  u32 green = RawInt(TupleGet(c, 1, mem));
  u32 blue = RawInt(TupleGet(c, 2, mem));
  return (red << 24) | (green << 16) | (blue << 8) | 0xFF;
}

static bool IsColor(Val color, Mem *mem)
{
  if (!IsTuple(color, mem)) return false;
  if (TupleLength(color, mem) != 3) return false;
  if (!IsInt(TupleGet(color, 0, mem))) return false;
  if (!IsInt(TupleGet(color, 1, mem))) return false;
  if (!IsInt(TupleGet(color, 2, mem))) return false;
  return true;
}
