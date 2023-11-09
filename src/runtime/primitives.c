#include "primitives.h"
#include "univ/math.h"
#include "univ/system.h"
#include <stdio.h>


static Result VMType(u32 num_args, VM *vm);
static Result VMHead(u32 num_args, VM *vm);
static Result VMTail(u32 num_args, VM *vm);
static Result VMPrint(u32 num_args, VM *vm);
static Result VMInspect(u32 num_args, VM *vm);
static Result VMOpen(u32 num_args, VM *vm);
static Result VMRead(u32 num_args, VM *vm);
static Result VMWrite(u32 num_args, VM *vm);
static Result VMTicks(u32 num_args, VM *vm);
static Result VMSeed(u32 num_args, VM *vm);
static Result VMRandom(u32 num_args, VM *vm);

static PrimitiveDef primitives[] = {
  {/*"typeof"*/     0x7FDA1EFC, &VMType},
  {/*"head"*/       0x7FD4F911, &VMHead},
  {/*"tail"*/       0x7FD16764, &VMTail},
  {/*"print"*/      0x7FD39F07, &VMPrint},
  {/*"inspect"*/    0x7FD068BD, &VMInspect},
  {/*"open"*/       0x7FD6E1B6, &VMOpen},
  {/*"read"*/       0x7FDEC06F, &VMRead},
  {/*"write"*/      0x7FDA9850, &VMWrite},
  {/*"ticks"*/      0x7FD145EA, &VMTicks},
  {/*"seed"*/       0x7FDCAC5E, &VMSeed},
  {/*"random"*/     0x7FD3FB29, &VMRandom}
};

Val PrimitiveEnv(Mem *mem)
{
  u32 i;
  Val frame = MakeTuple(ArrayCount(primitives), mem);
  for (i = 0; i < ArrayCount(primitives); i++) {
    Val primitive = Pair(Primitive, IntVal(i), mem);
    TupleSet(frame, i, primitive, mem);
  }
  return frame;
}

Val CompileEnv(Mem *mem)
{
  u32 i;
  Val frame = MakeTuple(ArrayCount(primitives), mem);
  for (i = 0; i < ArrayCount(primitives); i++) {
    TupleSet(frame, i, primitives[i].id, mem);
  }
  return frame;
}

Result DoPrimitive(Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(id)].fn(num_args, vm);
}

static Result CheckTypes(u32 num_args, u32 num_params, Val *types, VM *vm)
{
  u32 i;
  if (num_args != num_params) return RuntimeError("Arity error", vm);
  for (i = 0; i < num_params; i++) {
    if (types[i] == Nil) continue;
    if (TypeOf(StackRef(vm, i), &vm->mem) != types[i]) return RuntimeError("Type error", vm);
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

  return OkResult(TypeOf(arg, &vm->mem));
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
    } else if (IsBignum(value, &vm->mem)) {
      i64 num = ((u64*)(vm->mem.values + RawVal(value) + 1))[0];
      printf("%lld", num);
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
  } else if (IsBignum(value, mem)) {
    i64 num = ((u64*)(mem->values + RawVal(value) + 1))[0];
    printf("%lld", num);
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

  Val types[1] = {BinaryType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  name = StackPop(vm);

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

  if (!IsInt(size)) return RuntimeError("Type error", vm);

  if (!IsPair(ref)) return RuntimeError("Type error", vm);
  if (Head(ref, &vm->mem) != File) return RuntimeError("Type error", vm);

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

  if (Head(ref, &vm->mem) != File) return RuntimeError("Type error", vm);

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
  if (num_args != 0) return RuntimeError("Argument error", vm);
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
