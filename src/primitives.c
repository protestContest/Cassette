#include "primitives.h"
#include "env.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static Val VMPrint(u32 num_args, VM *vm);
static Val VMInspect(u32 num_args, VM *vm);
static Val VMOpen(u32 num_args, VM *vm);
static Val VMRead(u32 num_args, VM *vm);
static Val VMWrite(u32 num_args, VM *vm);

static PrimitiveDef primitives[] = {
  {"print", &VMPrint},
  {"inspect", &VMInspect},
  {"open", &VMOpen},
  {"read", &VMRead},
  {"write", &VMWrite}
};

PrimitiveDef *Primitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

Val DefinePrimitives(Mem *mem)
{
  u32 i;
  Val frame = MakeTuple(NumPrimitives(), mem);
  for (i = 0; i < NumPrimitives(); i++) {
    Val primitive = Pair(Primitive, IntVal(i), mem);
    TupleSet(frame, i, primitive, mem);
  }
  return frame;
}

Val DoPrimitive(Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(id)].fn(num_args, vm);
}

static Val VMPrint(u32 num_args, VM *vm)
{
  u32 i;

  for (i = 0; i < num_args; i++) {
    Val value = StackPop(vm);
    if (IsNum(value)) {
      PrintVal(value, 0);
    } else if (IsSym(value)) {
      PrintVal(value, &vm->symbols);
    } else if (IsBinary(value, &vm->mem)) {
      u32 len = BinaryLength(value, &vm->mem);
      printf("%*.*s", len, len, (char*)BinaryData(value, &vm->mem));
    } else {
      return Error;
    }
    if (i < num_args-1) printf(" ");
  }
  printf("\n");
  return Ok;
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
      PrintTail(value, depth, mem, symbols);
    }
  }
}

static void InspectPrint(Val value, u32 depth, Mem *mem, SymbolTable *symbols)
{
  if (IsNil(value)) {
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

static Val VMInspect(u32 num_args, VM *vm)
{
  Val value;
  if (num_args != 1) {
    vm->stack.count -= num_args;
    return Error;
  }

  value = StackPop(vm);

  InspectPrint(value, 10, &vm->mem, &vm->symbols);
  printf("\n");

  return Ok;
}

static Val VMOpen(u32 num_args, VM *vm)
{
  Val name;
  char filename[256];
  u32 name_len;
  int file;

  if (num_args < 1) return Error;

  name = StackPop(vm);
  vm->stack.count -= num_args-1;

  if (!IsBinary(name, &vm->mem)) return Error;

  name_len = Min(255, BinaryLength(name, &vm->mem));
  Copy(BinaryData(name, &vm->mem), filename, name_len);
  filename[name_len] = 0;

  file = Open(filename);
  if (file < 0) return Error;

  return Pair(File, IntVal(file), &vm->mem);
}

static Val VMRead(u32 num_args, VM *vm)
{
  Val ref, size, result;
  char *buf;
  i32 bytes_read;

  if (num_args < 1) return Error;

  ref = StackPop(vm);
  if (num_args > 1) {
    size = StackPop(vm);
    vm->stack.count -= num_args-2;
  } else {
    size = IntVal(1024);
    vm->stack.count -= num_args-1;
  }

  if (!IsPair(ref)) return Error;
  if (Head(ref, &vm->mem) != File) return Error;
  if (!IsInt(size)) return Error;

  buf = malloc(RawInt(size));
  bytes_read = read(RawInt(Tail(ref, &vm->mem)), buf, size);
  if (bytes_read < 0) return Error;
  if (bytes_read == 0) return Nil;

  result = BinaryFrom(buf, bytes_read, &vm->mem);
  free(buf);
  return result;
}

static Val VMWrite(u32 num_args, VM *vm)
{
  Val ref, binary;
  i32 written;
  char *buf;
  u32 bytes_left;
  int file;

  if (num_args < 2) return Error;

  binary = StackPop(vm);
  ref = StackPop(vm);
  vm->stack.count -= num_args-2;

  PrintVal(ref, 0);
  printf("\n");
  DumpMem(&vm->mem, 0);

  if (!IsPair(ref) || Head(ref, &vm->mem) != File) return Error;
  if (!IsBinary(binary, &vm->mem)) return Error;

  file = RawInt(Tail(ref, &vm->mem));
  buf = BinaryData(binary, &vm->mem);
  bytes_left = BinaryLength(binary, &vm->mem);

  while (bytes_left > 0) {
    written = write(file, buf, bytes_left);
    if (written < 0) return Error;
    bytes_left -= written;
  }

  return Ok;
}

Val CompileEnv(Mem *mem, SymbolTable *symbols)
{
  Val frame = MakeTuple(NumPrimitives(), mem);
  u32 i;
  for (i = 0; i < NumPrimitives(); i++) {
    TupleSet(frame, i, Sym(primitives[i].name, symbols), mem);
  }
  return ExtendEnv(Nil, frame, mem);
}
