#include "primitives.h"
#include "univ/math.h"
#include "univ/system.h"
#include "canvas/canvas.h"
#include "univ/str.h"
#include "univ/hash.h"
#include "compile/parse.h"
#include "compile/project.h"
#include "env.h"
#include <math.h>
#include <stdio.h>

static Result VMType(u32 num_args, VM *vm);
static Result VMHead(u32 num_args, VM *vm);
static Result VMTail(u32 num_args, VM *vm);
static Result VMMapGet(u32 num_args, VM *vm);
static Result VMMapSet(u32 num_args, VM *vm);
static Result VMMapKeys(u32 num_args, VM *vm);
static Result VMTrunc(u32 num_args, VM *vm);
static Result VMPrint(u32 num_args, VM *vm);
static Result VMInspect(u32 num_args, VM *vm);
static Result VMOpen(u32 num_args, VM *vm);
static Result VMRead(u32 num_args, VM *vm);
static Result VMWrite(u32 num_args, VM *vm);
static Result VMTicks(u32 num_args, VM *vm);
static Result VMSeed(u32 num_args, VM *vm);
static Result VMRandom(u32 num_args, VM *vm);
static Result VMCanvas(u32 num_args, VM *vm);
static Result VMCloseCanvas(u32 num_args, VM *vm);
static Result VMLine(u32 num_args, VM *vm);
static Result VMText(u32 num_args, VM *vm);

static PrimitiveDef kernel[] = {
  {/* typeof */   0x7FDA14D4, &VMType},
  {/* head */     0x7FD4FAFD, &VMHead},
  {/* tail */     0x7FD1655A, &VMTail},
  {/* mget */     0x7FDD69EF, &VMMapGet},
  {/* mset */     0x7FD53047, &VMMapSet},
  {/* mkeys */    0x7FD459E9, &VMMapKeys},
  {/* trunc */    0x7FD36865, &VMTrunc},
};

static PrimitiveDef io[] = {
  {/* print */    0x7FD3984B, &VMPrint},
  {/* inspect */  0x7FD06371, &VMInspect},
  {/* open */     0x7FD6E11B, &VMOpen},
  {/* read */     0x7FDEC474, &VMRead},
  {/* write */    0x7FDA90A8, &VMWrite},
};

static PrimitiveDef sys[] = {
  {/* ticks */    0x7FD14415, &VMTicks},
  {/* seed */     0x7FDCADD1, &VMSeed},
  {/* random */   0x7FD3FCF1, &VMRandom},
};

static PrimitiveDef canvas[] = {
  {/* new */      0x7FDEA6DA, &VMCanvas},
  {/* close */    0x7FDF88C9, &VMCloseCanvas},
  {/* line */     0x7FD0B46A, &VMLine},
  {/* text */     0x7FD2824B, &VMText},
};

static PrimitiveModuleDef primitives[] = {
  {/* Kernel */   KernelMod, ArrayCount(kernel), kernel},
  {/* IO */       0x7FDDDA86, ArrayCount(io), io},
  {/* Sys */      0x7FD9DBF5, ArrayCount(sys), sys},
  {/* Canvas */   0x7FD291D0, ArrayCount(canvas), canvas}
};

PrimitiveModuleDef *GetPrimitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

Result DoPrimitive(Val mod, Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(mod)].fns[RawInt(id)].fn(num_args, vm);
}

static Result CheckTypes(u32 num_args, u32 num_params, Val *types, VM *vm)
{
  u32 i;
  if (num_args != num_params) return RuntimeError("Arity error", vm);
  for (i = 0; i < num_params; i++) {
    if (types[i] == Nil) continue;
    if (types[i] == NumType) {
      if (TypeSym(StackRef(vm, i), &vm->mem) != IntType && TypeSym(StackRef(vm, i), &vm->mem) != FloatType) {
        return RuntimeError("Type error: Expected number", vm);
      }
    } else if (TypeSym(StackRef(vm, i), &vm->mem) != types[i]) {
      char *error = JoinStr("Type error: Expected", TypeName(types[i]), ' ');
      return RuntimeError(error, vm);
    }
  }
  return OkResult(Nil);
}

static Result VMType(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {Nil};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  vm->stack.count -= num_args-1;

  return OkResult(TypeSym(arg, &vm->mem));
}

static Result VMHead(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  return OkResult(Head(arg, &vm->mem));
}

static Result VMTail(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  return OkResult(Tail(arg, &vm->mem));
}

static Result VMMapGet(u32 num_args, VM *vm)
{
  Val key, map;
  Val types[2] = {Nil, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackPop(vm);
  map = StackPop(vm);
  return OkResult(MapGet(map, key, &vm->mem));
}

static Result VMMapSet(u32 num_args, VM *vm)
{
  Val key, value, map;
  Val types[3] = {Nil, Nil, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  key = StackPop(vm);
  map = StackPop(vm);
  return OkResult(MapSet(map, key, value, &vm->mem));
}

static Result VMMapKeys(u32 num_args, VM *vm)
{
  Val map;
  Val types[1] = {MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  map = StackPop(vm);
  return OkResult(MapKeys(map, Nil, &vm->mem));
}

static Result VMTrunc(u32 num_args, VM *vm)
{
  Val num;
  Val types[1] = {NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  num = StackPop(vm);
  return OkResult(IntVal((i32)RawNum(num)));
}

static Result VMPrint(u32 num_args, VM *vm)
{
  u32 i;

  for (i = 0; i < num_args; i++) {
    Val value = StackPop(vm);
    if (IsInt(value)) {
      printf("%d", RawInt(value));
    } else if (IsFloat(value)) {
      printf("%f", RawFloat(value));
    } else if (IsSym(value)) {
      printf("%s", SymbolName(value, &vm->chunk->symbols));
    } else if (IsBinary(value, &vm->mem)) {
      u32 len = BinaryLength(value, &vm->mem);
      printf("%*.*s", len, len, (char*)BinaryData(value, &vm->mem));
    } else {
      return RuntimeError("Unprintable value", vm);
    }
    if (i < num_args-1) printf(" ");
  }
  printf("\n");
  return OkResult(Ok);
}

static void InspectPrint(Val value, u32 depth, Mem *mem, SymbolTable *symbols);

static void PrintTail(Val value, u32 depth, Mem *mem, SymbolTable *symbols)
{
  Assert(IsPair(value));

  if (depth == 0) {
    printf("...]");
  } else {
    InspectPrint(Head(value, mem), depth-1, mem, symbols);
    value = Tail(value, mem);
    if (!IsPair(value)) {
      printf(" | ");
      InspectPrint(value, depth-1, mem, symbols);
      printf("]");
    } else if (value == Nil) {
      printf("]");
    } else {
      printf(" ");
      PrintTail(value, depth, mem, symbols);
    }
  }
}

static void InspectPrint(Val value, u32 depth, Mem *mem, SymbolTable *symbols)
{
  if (value == Nil) {
    printf("nil");
  } else if (value == True) {
    printf("true");
  } else if (value == False) {
    printf("false");
  } else if (IsInt(value)) {
    printf("%d", RawInt(value));
  } else if (IsFloat(value)) {
    printf("%f", RawFloat(value));
  } else if (IsSym(value)) {
    printf(":%s", SymbolName(value, symbols));
  } else if (IsPair(value)) {
    printf("[");
    if (Head(value, mem) == Function) {
      PrintTail(value, 1, mem, symbols);
    } else {
      PrintTail(value, depth, mem, symbols);
    }
  } else if (IsTuple(value, mem)) {
    u32 i;
    if (depth > 0) {
      printf("{");
      for (i = 0; i < TupleLength(value, mem); i++) {
        InspectPrint(TupleGet(value, i, mem), depth-1, mem, symbols);
      }
      printf("}");
    } else {
      printf("{...}");
    }
  } else if (IsBinary(value, mem)) {
    u32 size = BinaryLength(value, mem);
    char *str = BinaryData(value, mem);
    printf("\"%*.*s\"", size, size, str);
  } else if (IsMap(value, mem)) {
    u32 i;
    if (depth > 0) {
      printf("{: ");
      for (i = 0; i < MapCount(value, mem); i++) {
        InspectPrint(TupleGet(value, i, mem), depth-1, mem, symbols);
      }
      printf("}");
    } else {
      printf("{: ...}");
    }
  } else {
    printf("???");
  }
}

static Result VMInspect(u32 num_args, VM *vm)
{
  Val value;
  if (num_args != 1) {
    vm->stack.count -= num_args;
    return RuntimeError("Arity error", vm);
  }

  value = StackPop(vm);

  InspectPrint(value, 10, &vm->mem, &vm->chunk->symbols);
  printf("\n");

  return OkResult(value);
}

static Result VMOpen(u32 num_args, VM *vm)
{
  Val name;
  char filename[256];
  u32 name_len;
  int file;

  Val types[1] = {BinaryType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  name = StackPop(vm);

  name_len = Min(255, BinaryLength(name, &vm->mem));
  Copy(BinaryData(name, &vm->mem), filename, name_len);
  filename[name_len] = 0;

  file = CreateOrOpen(filename);
  if (file < 0) return OkResult(Error);

  return OkResult(Pair(File, IntVal(file), &vm->mem));
}

static Result VMRead(u32 num_args, VM *vm)
{
  Val ref, size, result;
  char *buf;
  i32 bytes_read;

  if (num_args < 1) return RuntimeError("Arity error", vm);

  ref = StackPop(vm);
  if (num_args > 1) {
    size = StackPop(vm);
  } else {
    size = IntVal(1024);
  }

  if (IsBinary(ref, &vm->mem)) {
    u32 name_len = Min(255, BinaryLength(ref, &vm->mem));
    char filename[256];
    int file;

    Copy(BinaryData(ref, &vm->mem), filename, name_len);
    filename[name_len] = 0;

    file = Open(filename);
    if (file < 0) return OkResult(Error);
    ref = Pair(File, IntVal(file), &vm->mem);

    if (num_args == 1) {
      size = IntVal(FileSize(file));
    }
  }

  if (!IsInt(size)) return RuntimeError("Type error: Expected integer", vm);

  if (!IsPair(ref)) return RuntimeError("Type error: Expected file reference", vm);
  if (Head(ref, &vm->mem) != File) return RuntimeError("Type error: Expected file reference", vm);

  buf = Alloc(RawInt(size));
  bytes_read = Read(RawInt(Tail(ref, &vm->mem)), buf, size);
  if (bytes_read < 0) return OkResult(Error);
  if (bytes_read == 0) return OkResult(Nil);

  result = BinaryFrom(buf, bytes_read, &vm->mem);
  Free(buf);
  return OkResult(result);
}

static Result VMWrite(u32 num_args, VM *vm)
{
  Val ref, binary;
  i32 written;
  char *buf;
  u32 bytes_left;
  int file;

  Val types[2] = {BinaryType, PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  binary = StackPop(vm);
  ref = StackPop(vm);

  if (Head(ref, &vm->mem) != File) return RuntimeError("Type error: Expected file reference", vm);

  file = RawInt(Tail(ref, &vm->mem));
  buf = BinaryData(binary, &vm->mem);
  bytes_left = BinaryLength(binary, &vm->mem);

  while (bytes_left > 0) {
    written = Write(file, buf, bytes_left);
    if (written < 0) return OkResult(Error);
    bytes_left -= written;
  }

  return OkResult(Ok);
}

static Result VMTicks(u32 num_args, VM *vm)
{
  if (num_args != 0) return RuntimeError("Arity error", vm);
  return OkResult(IntVal(Ticks()));
}

static Result VMSeed(u32 num_args, VM *vm)
{
  Val seed;
  Val types[1] = {IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  seed = StackPop(vm);
  Seed(RawInt(seed));
  return OkResult(Nil);
}

static Result VMRandom(u32 num_args, VM *vm)
{
  float r = (float)Random() / (float)MaxUInt;
  Result result = CheckTypes(num_args, 0, 0, vm);
  if (!result.ok) return result;

  return OkResult(FloatVal(r));
}

#ifdef CANVAS
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

static Result VMCanvas(u32 num_args, VM *vm)
{
  u32 width, height, id;
  Val context, font, color;
  Canvas *canvas;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  height = RawInt(StackPop(vm));
  width = RawInt(StackPop(vm));
  canvas = MakeCanvas(width, height, "Canvas");
  id = vm->canvases.count;
  ObjVecPush(&vm->canvases, canvas);

  font = BinaryFrom(canvas->font_filename, StrLen(canvas->font_filename), &vm->mem);
  color = MakeColor(canvas->color, &vm->mem);
  context = MakeMap(&vm->mem);
  context = MapSet(context, Sym("type", &vm->chunk->symbols), Sym("Canvas", &vm->chunk->symbols), &vm->mem);
  context = MapSet(context, Sym("id", &vm->chunk->symbols), IntVal(id), &vm->mem);
  context = MapSet(context, Sym("width", &vm->chunk->symbols), IntVal(width), &vm->mem);
  context = MapSet(context, Sym("height", &vm->chunk->symbols), IntVal(height), &vm->mem);
  context = MapSet(context, Sym("font", &vm->chunk->symbols), font, &vm->mem);
  context = MapSet(context, Sym("font-size", &vm->chunk->symbols), IntVal(canvas->font_size), &vm->mem);
  context = MapSet(context, Sym("color", &vm->chunk->symbols), color, &vm->mem);

  return OkResult(context);
}

static Result VMCloseCanvas(u32 num_args, VM *vm)
{
  Val context, id;
  Canvas *canvas;
  Val types[1] = {MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  context = StackPop(vm);

  if (MapGet(context, SymbolFor("type"), &vm->mem) != SymbolFor("Canvas")) {
    return RuntimeError("Not a canvas", vm);
  }
  id = MapGet(context, SymbolFor("id"), &vm->mem);

  if ((u32)RawInt(id) >= vm->canvases.count) return RuntimeError("Undefined canvas", vm);
  canvas = vm->canvases.items[RawInt(id)];

  FreeCanvas(canvas);

  return OkResult(Ok);
}

static Result VMLine(u32 num_args, VM *vm)
{
  Val x0, y0, x1, y1, context, id;
  Canvas *canvas;
  Val types[5] = {NumType, NumType, NumType, NumType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  y1 = StackPop(vm);
  x1 = StackPop(vm);
  y0 = StackPop(vm);
  x0 = StackPop(vm);
  context = StackPop(vm);

  if (MapGet(context, SymbolFor("type"), &vm->mem) != SymbolFor("Canvas")) {
    return RuntimeError("Not a canvas", vm);
  }
  id = MapGet(context, SymbolFor("id"), &vm->mem);

  if ((u32)RawInt(id) >= vm->canvases.count) return RuntimeError("Undefined canvas", vm);
  canvas = vm->canvases.items[RawInt(id)];

  canvas->color = ColorFrom(MapGet(context, SymbolFor("color"), &vm->mem), &vm->mem);

  DrawLine(RawNum(x0), RawNum(y0), RawNum(x1), RawNum(y1), canvas);

  return OkResult(Ok);
}

static Result VMText(u32 num_args, VM *vm)
{
  Val x, y, id, context, font;
  char *text;
  char *fontname;
  u32 font_size;
  Val binary;
  Canvas *canvas;
  Val types[4] = {NumType, NumType, BinaryType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  y = StackPop(vm);
  x = StackPop(vm);
  binary = StackPop(vm);
  context = StackPop(vm);

  if (MapGet(context, SymbolFor("type"), &vm->mem) != SymbolFor("Canvas")) {
    return RuntimeError("Not a canvas", vm);
  }
  id = MapGet(context, SymbolFor("id"), &vm->mem);

  if ((u32)RawInt(id) >= vm->canvases.count) return RuntimeError("Undefined canvas", vm);
  canvas = vm->canvases.items[RawInt(id)];

  canvas->color = ColorFrom(MapGet(context, SymbolFor("color"), &vm->mem), &vm->mem);
  font = MapGet(context, SymbolFor("font"), &vm->mem);
  fontname = CopyStr(BinaryData(font, &vm->mem), BinaryLength(font, &vm->mem));
  font_size = RawInt(MapGet(context, SymbolFor("font-size"), &vm->mem));
  if (canvas->font_size != font_size || !StrEq(canvas->font_filename, fontname)) {
    SetFont(canvas, fontname, font_size);
  }
  Free(fontname);

  text = CopyStr(BinaryData(binary, &vm->mem), BinaryLength(binary, &vm->mem));
  DrawText(text, RawNum(x), RawNum(y), canvas);
  Free(text);

  return OkResult(Ok);
}
#else
static Result VMCanvas(u32 num_args, VM *vm)
{
  u32 i;
  for (i = 0; i < num_args; i++) StackPop(vm);
  return OkResult(Error);
}

static Result VMLine(u32 num_args, VM *vm)
{
  u32 i;
  for (i = 0; i < num_args; i++) StackPop(vm);
  return OkResult(Error);
}

static Result VMText(u32 num_args, VM *vm)
{
  u32 i;
  for (i = 0; i < num_args; i++) StackPop(vm);
  return OkResult(Error);
}
#endif
