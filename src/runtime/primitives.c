#include "primitives.h"
#include "univ/math.h"
#include "univ/system.h"
#include "canvas/canvas.h"
#include "univ/str.h"
#include "univ/hash.h"
#include "compile/parse.h"
#include "compile/project.h"
#include "device/device.h"
#include "env.h"
#include <math.h>
#include <stdio.h>

static Result VMHead(u32 num_args, VM *vm);
static Result VMTail(u32 num_args, VM *vm);
static Result VMPanic(u32 num_args, VM *vm);
static Result VMUnwrap(u32 num_args, VM *vm);
static Result VMForceUnwrap(u32 num_args, VM *vm);
static Result VMOk(u32 num_args, VM *vm);

static Result VMOpen(u32 num_args, VM *vm);
static Result VMClose(u32 num_args, VM *vm);
static Result VMRead(u32 num_args, VM *vm);
static Result VMWrite(u32 num_args, VM *vm);
static Result VMGetParam(u32 num_args, VM *vm);
static Result VMSetParam(u32 num_args, VM *vm);

static Result VMType(u32 num_args, VM *vm);
static Result VMMapGet(u32 num_args, VM *vm);
static Result VMMapSet(u32 num_args, VM *vm);
static Result VMMapDelete(u32 num_args, VM *vm);
static Result VMMapKeys(u32 num_args, VM *vm);
static Result VMMapValues(u32 num_args, VM *vm);
static Result VMSplit(u32 num_args, VM *vm);
static Result VMJoinBin(u32 num_args, VM *vm);
static Result VMStuff(u32 num_args, VM *vm);
static Result VMTrunc(u32 num_args, VM *vm);
static Result VMSymName(u32 num_args, VM *vm);

static PrimitiveDef kernel[] = {
  {/* head */         0x7FD4FAFD, &VMHead},
  {/* tail */         0x7FD1655A, &VMTail},
  {/* panic! */       0x7FDA4AE9, &VMPanic},
  {/* unwrap */       0x7FDC5932, &VMUnwrap},
  {/* unwrap! */      0x7FDC1BBA, &VMForceUnwrap},
  {/* ok? */          0x7FD3025E, &VMOk},
};

static PrimitiveDef device[] = {
  {/* open */         0x7FD6E11B, &VMOpen},
  {/* close */        0x7FDF88C9, &VMClose},
  {/* read */         0x7FDEC474, &VMRead},
  {/* write */        0x7FDA90A8, &VMWrite},
  {/* get-param */    0x7FDE696B, &VMGetParam},
  {/* set-param */    0x7FDB637C, &VMSetParam},
};

static PrimitiveDef type[] = {
  {/* typeof */       0x7FDA14D4, &VMType},
  {/* map-get */      0x7FD781D0, &VMMapGet},
  {/* map-set */      0x7FDFD878, &VMMapSet},
  {/* map-del */      0x7FD330D3, &VMMapDelete},
  {/* map-keys */     0x7FD18996, &VMMapKeys},
  {/* map-values */   0x7FDC7EF0, &VMMapValues},
  {/* split-bin */    0x7FD24E81, &VMSplit},
  {/* join-bin */     0x7FD1C3BB, &VMJoinBin},
  {/* stuff */        0x7FD2CC7F, &VMStuff},
  {/* trunc */        0x7FD36865, &VMTrunc},
  {/* symbol-name */  0x7FD0CEDC, &VMSymName},
};

static PrimitiveModuleDef primitives[] = {
  {/* Kernel */       KernelMod, ArrayCount(kernel), kernel},
  {/* Device */       0x7FDBC2CD, ArrayCount(device), device},
  {/* Type */         0x7FDE0D53, ArrayCount(type), type},
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

static char *TypeName(Val type)
{
  switch (type) {
  case FloatType: return "float";
  case IntType: return "integer";
  case SymType: return "symbol";
  case PairType: return "pair";
  case ObjType: return "object";
  case TupleType: return "tuple";
  case BinaryType: return "binary";
  case MapType: return "map";
  case BignumType: return "bignum";
  default: return "unknown";
  }
}

static Result CheckTypes(u32 num_args, u32 num_params, Val *types, VM *vm)
{
  u32 i;
  if (num_args != num_params) return RuntimeError("Arity error", vm);
  for (i = 0; i < num_params; i++) {
    if (types[i] == Nil) continue;
    if (types[i] == NumType) {
      if (!IsInt(StackRef(vm, i)) && !IsFloat(StackRef(vm, i))) {
        return RuntimeError("Type error: Expected number", vm);
      }
    } else if (types[i] == FloatType) {
      if (!IsFloat(StackRef(vm, i))) {
        return RuntimeError("Type error: Expected float", vm);
      }
    } else if (TypeOf(StackRef(vm, i)) == ObjType) {
      Val header = MemRef(&vm->mem, RawVal(StackRef(vm, i)));
      if (TypeOf(header) != types[i]) {
        char *error = JoinStr("Type error: Expected", TypeName(types[i]), ' ');
        return RuntimeError(error, vm);
      }
    } else if (TypeOf(StackRef(vm, i)) != types[i]) {
      char *error = JoinStr("Type error: Expected", TypeName(types[i]), ' ');
      return RuntimeError(error, vm);
    }
  }
  return OkResult(Nil);
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

static Result VMPanic(u32 num_args, VM *vm)
{
  Val msg;
  Val types[1] = {Nil};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  msg = StackPop(vm);
  if (IsBinary(msg, &vm->mem)) {
    return RuntimeError(CopyStr(BinaryData(msg, &vm->mem), BinaryLength(msg, &vm->mem)), vm);
  } else {
    return RuntimeError("Unknown error", vm);
  }
}

static Result VMUnwrap(u32 num_args, VM *vm)
{
  Val value, default_val;
  Val types[2] = {PairType, Nil};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  default_val = StackPop(vm);
  value = StackPop(vm);
  if (Head(value, &vm->mem) == Ok) {
    return OkResult(Tail(value, &vm->mem));
  } else {
    return OkResult(default_val);
  }
}

static Result VMForceUnwrap(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  if (Head(value, &vm->mem) == Ok) {
    return OkResult(Tail(value, &vm->mem));
  } else {
    Val error = Tail(value, &vm->mem);
    if (IsBinary(error, &vm->mem)) {
      return RuntimeError(CopyStr(BinaryData(error, &vm->mem), BinaryLength(error, &vm->mem)), vm);
    } else {
      return RuntimeError("Unwrap error", vm);
    }
  }
}

static Result VMOk(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(Head(value, &vm->mem) == Ok));
}

static Result VMOpen(u32 num_args, VM *vm)
{
  Val opts;
  DeviceType type;
  u32 id;
  Val types[2] = {TupleType, SymType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  opts = StackPop(vm);
  type = GetDeviceType(StackPop(vm));
  id = PopCount(RightZeroBit(vm->dev_map) - 1);

  if (type == (u32)-1) {
    return OkError(ErrorResult("Invalid device type", 0, 0), &vm->mem);
  }

  if (id > ArrayCount(vm->devices)) {
    return OkError(ErrorResult("Too many open devices", 0, 0), &vm->mem);
  }

  result = DeviceOpen(type, opts, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  vm->dev_map |= Bit(id);
  vm->devices[id].type = type;
  vm->devices[id].context = result.data;

  return OkResult(Pair(Ok, IntVal(id), &vm->mem));
}

static Result VMClose(u32 num_args, VM *vm)
{
  u32 id;
  Val types[1] = {IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceClose(&vm->devices[id], &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  vm->dev_map &= ~Bit(id);

  return OkResult(Nil);
}

static Result VMRead(u32 num_args, VM *vm)
{
  u32 id;
  Val length;
  Val types[2] = {Nil, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  length = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceRead(&vm->devices[id], length, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return OkResult(Pair(Ok, result.value, &vm->mem));
}

static Result VMWrite(u32 num_args, VM *vm)
{
  u32 id;
  Val data;
  Val types[2] = {Nil, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  data = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceWrite(&vm->devices[id], data, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return OkResult(Pair(Ok, result.value, &vm->mem));
}

static Result VMGetParam(u32 num_args, VM *vm)
{
  u32 id;
  Val key;
  Val types[2] = {SymType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceGet(&vm->devices[id], key, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return OkResult(Pair(Ok, result.value, &vm->mem));
}

static Result VMSetParam(u32 num_args, VM *vm)
{
  u32 id;
  Val key, value;
  Val types[3] = {Nil, SymType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  key = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceSet(&vm->devices[id], key, value, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return OkResult(Pair(Ok, result.value, &vm->mem));
}

static Result VMType(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {Nil};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  vm->stack.count -= num_args-1;

  if (IsFloat(arg)) {
    return OkResult(FloatType);
  } else if (IsFunction(arg, &vm->mem) || IsPrimitive(arg, &vm->mem)) {
    return OkResult(Sym("function", &vm->chunk->symbols));
  }

  if (IsObj(arg)) arg = MemRef(&vm->mem, RawVal(arg));

  return OkResult(Sym(TypeName(TypeOf(arg)), &vm->chunk->symbols));
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

static Result VMMapDelete(u32 num_args, VM *vm)
{
  Val key, map;
  Val types[2] = {Nil, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackPop(vm);
  map = StackPop(vm);
  return OkResult(MapDelete(map, key, &vm->mem));
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

static Result VMMapValues(u32 num_args, VM *vm)
{
  Val map;
  Val types[1] = {MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  map = StackPop(vm);
  return OkResult(MapValues(map, Nil, &vm->mem));
}

static Result VMSplit(u32 num_args, VM *vm)
{
  Val bin, index;
  Val types[2] = {IntType, BinaryType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  index = StackPop(vm);
  bin = StackPop(vm);
  if (RawInt(index) <= 0) {
    return OkResult(Pair(MakeBinary(0, &vm->mem), bin, &vm->mem));
  } else if ((u32)RawInt(index) >= BinaryLength(bin, &vm->mem)) {
    return OkResult(Pair(bin, MakeBinary(0, &vm->mem), &vm->mem));
  } else {
    u32 tail_len = BinaryLength(bin, &vm->mem) - RawInt(index);
    Val head = BinaryFrom(BinaryData(bin, &vm->mem), RawInt(index), &vm->mem);
    Val tail = BinaryFrom(BinaryData(bin, &vm->mem) + RawInt(index), tail_len, &vm->mem);
    return OkResult(Pair(head, tail, &vm->mem));
  }
}

static Result IODataLength(Val iodata, VM *vm)
{
  if (iodata == Nil) {
    return OkResult(0);
  } else if (IsBinary(iodata, &vm->mem)) {
    return OkResult(BinaryLength(iodata, &vm->mem));
  } else if (IsInt(iodata)) {
    if (RawInt(iodata) < 0 && RawInt(iodata) > 255) {
      return RuntimeError("Type error: Expected iodata", vm);
    }
    return OkResult(1);
  } else if (IsPair(iodata)) {
    Result head, tail;
    head = IODataLength(Head(iodata, &vm->mem), vm);
    if (!head.ok) return head;
    tail = IODataLength(Tail(iodata, &vm->mem), vm);
    if (!tail.ok) return tail;
    return OkResult(head.value + tail.value);
  } else {
    return RuntimeError("Type error: Expected iodata", vm);
  }
}

static u32 CopyIOData(Val iodata, char *buf, u32 index, Mem *mem)
{
  if (iodata == Nil) {
    return 0;
  } else if (IsBinary(iodata, mem)) {
    Copy(BinaryData(iodata, mem), buf+index, BinaryLength(iodata, mem));
    return BinaryLength(iodata, mem);
  } else if (IsInt(iodata)) {
    buf[index] = RawInt(iodata);
    return 1;
  } else if (IsPair(iodata)) {
    u32 head = CopyIOData(Head(iodata, mem), buf, index, mem);
    u32 tail = CopyIOData(Tail(iodata, mem), buf, index + head, mem);
    return head + tail;
  } else {
    return 0;
  }
}

static Result VMJoinBin(u32 num_args, VM *vm)
{
  Val iodata, bin;
  Val types[1] = {Nil};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  iodata = StackPop(vm);
  if (IsBinary(iodata, &vm->mem)) {
    return OkResult(iodata);
  } else {
    result = IODataLength(iodata, vm);
    if (!result.ok) return OkResult(Error);

    bin = MakeBinary(result.value, &vm->mem);
    CopyIOData(iodata, BinaryData(bin, &vm->mem), 0, &vm->mem);
    return OkResult(bin);
  }
}

static Result VMStuff(u32 num_args, VM *vm)
{
  Val iodata, bin;
  char *data;
  u32 i;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  iodata = StackPop(vm);
  bin = MakeBinary(ListLength(iodata, &vm->mem), &vm->mem);
  data = BinaryData(bin, &vm->mem);
  for (i = 0; i < BinaryLength(bin, &vm->mem); i++) {
    u8 byte = RawInt(Head(iodata, &vm->mem));
    iodata = Tail(iodata, &vm->mem);
    data[i] = byte;
  }

  return OkResult(bin);
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

static Result VMSymName(u32 num_args, VM *vm)
{
  Val sym;
  char *name;
  Val types[1] = {SymType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  sym = StackPop(vm);
  name = SymbolName(sym, &vm->chunk->symbols);
  return OkResult(BinaryFrom(name, StrLen(name), &vm->mem));
}
