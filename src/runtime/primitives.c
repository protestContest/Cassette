#include "primitives.h"
#include "env.h"
#include "univ/system.h"
#include "univ/math.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Result VMType(u32 num_args, VM *vm);
static Result VMHead(u32 num_args, VM *vm);
static Result VMTail(u32 num_args, VM *vm);
static Result VMGet(u32 num_args, VM *vm);
static Result VMInto(u32 num_args, VM *vm);
static Result VMCat(u32 num_args, VM *vm);
static Result VMPrint(u32 num_args, VM *vm);
static Result VMInspect(u32 num_args, VM *vm);
static Result VMOpen(u32 num_args, VM *vm);
static Result VMRead(u32 num_args, VM *vm);
static Result VMReadFile(u32 num_args, VM *vm);
static Result VMWrite(u32 num_args, VM *vm);
static Result VMNewline(u32 num_args, VM *vm);
static Result VMTicks(u32 num_args, VM *vm);
static Result VMSeed(u32 num_args, VM *vm);
static Result VMRandom(u32 num_args, VM *vm);

static PrimitiveDef primitives[] = {
  {"typeof", &VMType},
  {"head", &VMHead},
  {"tail", &VMTail},
  {"get", &VMGet},
  {"into", &VMInto},
  {"cat", &VMCat},
  {"print", &VMPrint},
  {"inspect", &VMInspect},
  {"open", &VMOpen},
  {"read", &VMRead},
  {"read_file", &VMReadFile},
  {"write", &VMWrite},
  {"newline", &VMNewline},
  {"ticks", &VMTicks},
  {"seed", &VMSeed},
  {"random", &VMRandom}
};

PrimitiveDef *Primitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

Val DefinePrimitives(Mem *mem, SymbolTable *symbols)
{
  u32 i;
  Val frame = MakeTuple(NumPrimitives(), mem);
  for (i = 0; i < NumPrimitives(); i++) {
    Val primitive = Pair(Primitive, IntVal(i), mem);
    if (symbols) Sym(primitives[i].name, symbols);
    TupleSet(frame, i, primitive, mem);
  }
  return frame;
}

Val CompileEnv(Mem *mem, SymbolTable *symbols)
{
  u32 i;
  Val frame = MakeTuple(NumPrimitives(), mem);
  for (i = 0; i < NumPrimitives(); i++) {
    Val name = Sym(primitives[i].name, symbols);
    TupleSet(frame, i, name, mem);
  }
  return frame;
}

Result DoPrimitive(Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(id)].fn(num_args, vm);
}

static Result VMType(u32 num_args, VM *vm)
{
  Val arg;
  if (num_args == 0) return RuntimeError("Argument error", vm);
  arg = StackPop(vm);
  vm->stack.count -= num_args-1;

  if (IsFloat(arg)) {
    return OkResult(Sym("float", &vm->chunk->symbols));
  } else if (IsInt(arg)) {
    return OkResult(Sym("integer", &vm->chunk->symbols));
  } else if (IsSym(arg)) {
    return OkResult(Sym("symbol", &vm->chunk->symbols));
  } else if (IsPair(arg)) {
    return OkResult(Sym("pair", &vm->chunk->symbols));
  } else if (IsTuple(arg, &vm->mem)) {
    return OkResult(Sym("tuple", &vm->chunk->symbols));
  } else if (IsBinary(arg, &vm->mem)) {
    return OkResult(Sym("binary", &vm->chunk->symbols));
  } else {
    return RuntimeError("Unknown value", vm);
  }
}

static Result VMHead(u32 num_args, VM *vm)
{
  Val arg;
  if (num_args != 1) return RuntimeError("Argument error", vm);
  arg = StackPop(vm);
  if (!IsPair(arg)) return RuntimeError("Type error", vm);
  return OkResult(Head(arg, &vm->mem));
}

static Result VMTail(u32 num_args, VM *vm)
{
  Val arg;
  if (num_args != 1) return RuntimeError("Argument error", vm);
  arg = StackPop(vm);
  if (!IsPair(arg)) return RuntimeError("Type error", vm);
  return OkResult(Tail(arg, &vm->mem));
}

static Result VMGet(u32 num_args, VM *vm)
{
  Val obj, index;
  if (num_args != 2) return RuntimeError("Argument error", vm);

  index = StackPop(vm);
  obj = StackPop(vm);
  if (!IsInt(index) || RawInt(index) < 0) return RuntimeError("Type error", vm);

  if (IsPair(obj)) {
    u32 i = RawInt(index);
    while (obj != Nil) {
      if (i == 0) {
        return OkResult(Head(obj, &vm->mem));
      }
      i--;
      obj = Tail(obj, &vm->mem);
    }
    return RuntimeError("Out of bounds", vm);
  } else if (IsTuple(obj, &vm->mem)) {
    if (RawInt(index) < 0 || (u32)RawInt(index) >= TupleLength(obj, &vm->mem)) {
      return RuntimeError("Out of bounds", vm);
    }
    return OkResult(TupleGet(obj, RawInt(index), &vm->mem));
  } else if (IsBinary(obj, &vm->mem)) {
    u8 byte;
    if (RawInt(index) < 0 || (u32)RawInt(index) >= BinaryLength(obj, &vm->mem)) {
      return RuntimeError("Out of bounds", vm);
    }
    byte = ((u8*)BinaryData(obj, &vm->mem))[RawInt(index)];
    return OkResult(IntVal(byte));
  } else {
    return RuntimeError("Type error", vm);
  }
}

static Result VMInto(u32 num_args, VM *vm)
{
  Val obj, container;
  if (num_args != 2) return RuntimeError("Argument error", vm);

  container = StackPop(vm);
  obj = StackPop(vm);

  if (IsPair(obj)) {
    if (IsPair(container)) {
      return OkResult(obj);
    } else if (IsTuple(container, &vm->mem)) {
      u32 i = 0;
      container = MakeTuple(ListLength(obj, &vm->mem), &vm->mem);
      while (obj != Nil) {
        TupleSet(container, i, Head(obj, &vm->mem), &vm->mem);
        i++;
      }
      return OkResult(container);
    } else {
      return RuntimeError("Type error", vm);
    }
  } else if (IsTuple(obj, &vm->mem)) {
    if (IsPair(container)) {
      u32 i;
      Val list = Nil;
      for (i = 0; i < TupleLength(obj, &vm->mem); i++) {
        Val item = TupleGet(obj, TupleLength(obj, &vm->mem) - i - 1, &vm->mem);
        list = Pair(item, list, &vm->mem);
      }
      return OkResult(list);
    } else if (IsTuple(container, &vm->mem)) {
      return OkResult(obj);
    } else {
      return RuntimeError("Type error", vm);
    }
  } else if (IsBinary(obj, &vm->mem)) {
    if (IsPair(container)) {
      u32 i;
      u32 length = BinaryLength(obj, &vm->mem);
      Val list = Nil;
      u8 *data = BinaryData(obj, &vm->mem);
      for (i = 0; i < BinaryLength(obj, &vm->mem); i++) {
        list = Pair(IntVal(data[length-i-1]), list, &vm->mem);
      }
      return OkResult(list);
    } else if (IsTuple(container, &vm->mem)) {
      u32 i;
      u32 length = BinaryLength(obj, &vm->mem);
      Val tuple = MakeTuple(length, &vm->mem);
      u8 *data = BinaryData(obj, &vm->mem);
      for (i = 0; i < BinaryLength(obj, &vm->mem); i++) {
        TupleSet(tuple, i, IntVal(data[i]), &vm->mem);
      }
      return OkResult(tuple);
    } else {
      return RuntimeError("Type error", vm);
    }
  } else {
    return RuntimeError("Type error", vm);
  }
}

static Result VMCat(u32 num_args, VM *vm)
{
  Val str1, str2, result;
  u32 len1, len2;
  u8 *data;
  if (num_args != 2) return RuntimeError("Argument error", vm);

  str2 = StackPop(vm);
  str1 = StackPop(vm);

  if (IsInt(str1)) {
    if (RawInt(str1) < 0 || RawInt(str1) > 255) return RuntimeError("Type error", vm);
  } else if (!IsBinary(str1, &vm->mem)) {
    return RuntimeError("Type error", vm);
  }

  if (IsInt(str2)) {
    if (RawInt(str2) < 0 || RawInt(str2) > 255) return RuntimeError("Type error", vm);
  } else if (!IsBinary(str2, &vm->mem)) {
    return RuntimeError("Type error", vm);
  }

  len1 = IsInt(str1) ? 1 : BinaryLength(str1, &vm->mem);
  len2 = IsInt(str2) ? 1 : BinaryLength(str2, &vm->mem);

  result = MakeBinary(len1 + len2, &vm->mem);
  data = BinaryData(result, &vm->mem);

  if (IsInt(str1)) {
    data[0] = RawInt(str1) & 0xFF;
    data++;
  } else {
    Copy(BinaryData(str1, &vm->mem), data, len1);
    data += len1;
  }

  if (IsInt(str2)) {
    data[0] = RawInt(str2) & 0xFF;
  } else {
    Copy(BinaryData(str2, &vm->mem), data, len2);
  }

  return OkResult(result);
}

static Result VMPrint(u32 num_args, VM *vm)
{
  u32 i;

  for (i = 0; i < num_args; i++) {
    Val value = StackPop(vm);
    if (IsNum(value)) {
      PrintVal(value, 0);
    } else if (IsSym(value)) {
      PrintVal(value, &vm->chunk->symbols);
    } else if (IsBinary(value, &vm->mem)) {
      u32 len = BinaryLength(value, &vm->mem);
      printf("%*.*s", len, len, (char*)BinaryData(value, &vm->mem));
    } else {
      return RuntimeError("Type error", vm);
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
  } else if (IsNum(value)) {
    PrintVal(value, 0);
  } else if (IsSym(value)) {
    printf(":");
    PrintVal(value, symbols);
  } else if (IsPair(value)) {
    printf("[");
    PrintTail(value, depth, mem, symbols);
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
  }
}

static Result VMInspect(u32 num_args, VM *vm)
{
  Val value;
  if (num_args != 1) {
    vm->stack.count -= num_args;
    return RuntimeError("Argument error", vm);
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

  if (num_args < 1) return RuntimeError("Argument error", vm);

  name = StackPop(vm);
  vm->stack.count -= num_args-1;

  if (!IsBinary(name, &vm->mem)) return RuntimeError("Type error", vm);

  name_len = Min(255, BinaryLength(name, &vm->mem));
  Copy(BinaryData(name, &vm->mem), filename, name_len);
  filename[name_len] = 0;

  file = Open(filename);
  if (file < 0) return OkResult(Error);

  return OkResult(Pair(File, IntVal(file), &vm->mem));
}

static Result VMRead(u32 num_args, VM *vm)
{
  Val ref, size, result;
  char *buf;
  i32 bytes_read;

  if (num_args < 1) return RuntimeError("Argument error", vm);

  ref = StackPop(vm);
  if (num_args > 1) {
    size = StackPop(vm);
    vm->stack.count -= num_args-2;
  } else {
    size = IntVal(1024);
    vm->stack.count -= num_args-1;
  }

  if (!IsPair(ref)) return RuntimeError("Type error", vm);
  if (Head(ref, &vm->mem) != File) return RuntimeError("Type error", vm);
  if (!IsInt(size)) return RuntimeError("Type error", vm);

  buf = malloc(RawInt(size));
  bytes_read = read(RawInt(Tail(ref, &vm->mem)), buf, size);
  if (bytes_read < 0) return OkResult(Error);
  if (bytes_read == 0) return OkResult(Nil);

  result = BinaryFrom(buf, bytes_read, &vm->mem);
  free(buf);
  return OkResult(result);
}

static Result VMReadFile(u32 num_args, VM *vm)
{
  Val name, result;
  u32 name_len, file_size, bytes_read;
  char filename[256];
  char *buf;
  int file;

  if (num_args < 1) return RuntimeError("Argument error", vm);

  name = StackPop(vm);
  vm->stack.count -= num_args-1;

  if (!IsBinary(name, &vm->mem)) return RuntimeError("Type error", vm);

  name_len = Min(255, BinaryLength(name, &vm->mem));
  Copy(BinaryData(name, &vm->mem), filename, name_len);
  filename[name_len] = 0;

  file = Open(filename);
  if (file < 0) return OkResult(Error);

  file_size = FileSize(file);

  buf = malloc(file_size);
  bytes_read = read(file, buf, file_size);
  if (bytes_read < 0) return OkResult(Error);
  if (bytes_read == 0) return OkResult(Nil);

  result = BinaryFrom(buf, bytes_read, &vm->mem);
  free(buf);
  return OkResult(result);
}

static Result VMWrite(u32 num_args, VM *vm)
{
  Val ref, binary;
  i32 written;
  char *buf;
  u32 bytes_left;
  int file;

  if (num_args < 2) return RuntimeError("Argument error", vm);

  binary = StackPop(vm);
  ref = StackPop(vm);
  vm->stack.count -= num_args-2;

  PrintVal(ref, 0);
  printf("\n");
  DumpMem(&vm->mem, 0);

  if (!IsPair(ref) || Head(ref, &vm->mem) != File) return RuntimeError("Type error", vm);
  if (!IsBinary(binary, &vm->mem)) return RuntimeError("Type error", vm);

  file = RawInt(Tail(ref, &vm->mem));
  buf = BinaryData(binary, &vm->mem);
  bytes_left = BinaryLength(binary, &vm->mem);

  while (bytes_left > 0) {
    written = write(file, buf, bytes_left);
    if (written < 0) return OkResult(Error);
    bytes_left -= written;
  }

  return OkResult(Ok);
}

static Result VMNewline(u32 num_args, VM *vm)
{
  if (num_args != 0) return RuntimeError("Argument error", vm);
  return OkResult(IntVal('\n'));
}

static Result VMTicks(u32 num_args, VM *vm)
{
  if (num_args != 0) return RuntimeError("Argument error", vm);
  return OkResult(IntVal(Ticks()));
}

static Result VMSeed(u32 num_args, VM *vm)
{
  Val seed;
  if (num_args != 1) return RuntimeError("Argument error", vm);
  seed = StackPop(vm);
  Seed(RawInt(seed));
  return OkResult(Nil);
}

static Result VMRandom(u32 num_args, VM *vm)
{
  float r = (float)Random() / (float)MaxUInt;
  if (num_args == 0) {
    return OkResult(FloatVal(r));
  } else if (num_args == 1) {
    Val max = StackPop(vm);
    if (!IsInt(max)) return RuntimeError("Type error", vm);
    return OkResult(IntVal(Floor(r * RawInt(max))));
  } else if (num_args == 2) {
    Val max = StackPop(vm);
    Val min = StackPop(vm);
    u32 range;
    if (!IsInt(max)) return RuntimeError("Type error", vm);
    if (!IsInt(min)) return RuntimeError("Type error", vm);
    range = RawInt(max) - RawInt(min);
    return OkResult(IntVal(Floor(r * range) + RawInt(min)));
  } else {
    return RuntimeError("Argument error", vm);
  }
}
