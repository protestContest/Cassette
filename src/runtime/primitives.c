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
static Result VMIsFloat(u32 num_args, VM *vm);
static Result VMIsInt(u32 num_args, VM *vm);
static Result VMIsSym(u32 num_args, VM *vm);
static Result VMIsPair(u32 num_args, VM *vm);
static Result VMIsTuple(u32 num_args, VM *vm);
static Result VMIsBin(u32 num_args, VM *vm);
static Result VMIsMap(u32 num_args, VM *vm);
static Result VMIsFunc(u32 num_args, VM *vm);

static Result VMNotEqual(u32 num_args, VM *vm);
static Result VMLessEqual(u32 num_args, VM *vm);
static Result VMGreaterEqual(u32 num_args, VM *vm);
static Result VMNot(u32 num_args, VM *vm);
static Result VMSub(u32 num_args, VM *vm);
static Result VMAdd(u32 num_args, VM *vm);
static Result VMBitNot(u32 num_args, VM *vm);
static Result VMLen(u32 num_args, VM *vm);
static Result VMMul(u32 num_args, VM *vm);
static Result VMDiv(u32 num_args, VM *vm);
static Result VMRem(u32 num_args, VM *vm);
static Result VMShiftLeft(u32 num_args, VM *vm);
static Result VMShiftRight(u32 num_args, VM *vm);
static Result VMBitAnd(u32 num_args, VM *vm);
static Result VMBitOr(u32 num_args, VM *vm);
static Result VMIn(u32 num_args, VM *vm);
static Result VMLess(u32 num_args, VM *vm);
static Result VMGreater(u32 num_args, VM *vm);
static Result VMEqual(u32 num_args, VM *vm);
static Result VMPair(u32 num_args, VM *vm);
static Result VMCat(u32 num_args, VM *vm);

static Result VMOpen(u32 num_args, VM *vm);
static Result VMClose(u32 num_args, VM *vm);
static Result VMRead(u32 num_args, VM *vm);
static Result VMWrite(u32 num_args, VM *vm);
static Result VMGetParam(u32 num_args, VM *vm);
static Result VMSetParam(u32 num_args, VM *vm);

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

static PrimitiveDef primitives[] = {
  {"head",            0x7FD4FAFD, &VMHead},
  {"tail",            0x7FD1655A, &VMTail},
  {"panic!",          0x7FDA4AE9, &VMPanic},
  {"unwrap",          0x7FDC5932, &VMUnwrap},
  {"unwrap!",         0x7FDC1BBA, &VMForceUnwrap},
  {"ok?",             0x7FD3025E, &VMOk},
  {"float?",          0x7FD6E4F4, &VMIsFloat},
  {"integer?",        0x7FDAB1E8, &VMIsInt},
  {"symbol?",         0x7FD46DF2, &VMIsSym},
  {"pair?",           0x7FDCEEC5, &VMIsPair},
  {"tuple?",          0x7FDE7376, &VMIsTuple},
  {"binary?",         0x7FD265F5, &VMIsBin},
  {"map?",            0x7FDE5C7F, &VMIsMap},
  {"function?",       0x7FD03556, &VMIsFunc},
  {"!=",              0x7FDD5C4E, &VMNotEqual},
  {"#",               0x7FDF82DB, &VMLen},
  {"%",               0x7FD3E679, &VMRem},
  {"&",               0x7FDB283C, &VMBitAnd},
  {"*",               0x7FD9A24B, &VMMul},
  {"+",               0x7FD26AB0, &VMAdd},
  {"-",               0x7FD9FBF9, &VMSub},
  {"/",               0x7FDDA21C, &VMDiv},
  {"<",               0x7FDD1F00, &VMLess},
  {"<<",              0x7FD72101, &VMShiftLeft},
  {"<=",              0x7FDE01F2, &VMLessEqual},
  {"<>",              0x7FD3C54B, &VMCat},
  {"==",              0x7FDC5014, &VMEqual},
  {">",               0x7FD9FB4A, &VMGreater},
  {">=",              0x7FD7C966, &VMGreaterEqual},
  {">>",              0x7FDA0DDF, &VMShiftRight},
  {"^",               0x7FDC17FE, &VMBitOr},
  {"in",              0x7FD98998, &VMIn},
  {"not",             0x7FDBCA20, &VMNot},
  {"|",               0x7FDA1ADB, &VMPair},
  {"~",               0x7FD373CF, &VMBitNot},
  {"open",            0x7FD6E11B, &VMOpen},
  {"close",           0x7FDF88C9, &VMClose},
  {"read",            0x7FDEC474, &VMRead},
  {"write",           0x7FDA90A8, &VMWrite},
  {"get-param",       0x7FDE696B, &VMGetParam},
  {"set-param",       0x7FDB637C, &VMSetParam},
  {"map-get",         0x7FD781D0, &VMMapGet},
  {"map-set",         0x7FDFD878, &VMMapSet},
  {"map-del",         0x7FD330D3, &VMMapDelete},
  {"map-keys",        0x7FD18996, &VMMapKeys},
  {"map-values",      0x7FDC7EF0, &VMMapValues},
  {"split-bin",       0x7FD24E81, &VMSplit},
  {"join-bin",        0x7FD1C3BB, &VMJoinBin},
  {"stuff",           0x7FD2CC7F, &VMStuff},
  {"trunc",           0x7FD36865, &VMTrunc},
  {"symbol-name",     0x7FD0CEDC, &VMSymName},
};

PrimitiveDef *GetPrimitives(void)
{
  return primitives;
}

u32 NumPrimitives(void)
{
  return ArrayCount(primitives);
}

Result DoPrimitive(Val id, u32 num_args, VM *vm)
{
  return primitives[RawInt(id)].fn(num_args, vm);
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
  case FuncType: return "function";
  default: return "unknown";
  }
}

static Result CheckTypes(u32 num_args, u32 num_params, Val *types, VM *vm)
{
  u32 i;
  if (num_args != num_params) return RuntimeError("Wrong number of arguments", vm);
  for (i = 0; i < num_params; i++) {
    if (types[i] == AnyType) continue;
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
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  msg = StackPop(vm);
  if (IsBinary(msg, &vm->mem)) {
    return RuntimeError(CopyStr(BinaryData(msg, &vm->mem), BinaryCount(msg, &vm->mem)), vm);
  } else {
    return RuntimeError("Unknown error", vm);
  }
}

static Result VMUnwrap(u32 num_args, VM *vm)
{
  Val value, default_val;
  Val types[2] = {PairType, AnyType};
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
      return RuntimeError(CopyStr(BinaryData(error, &vm->mem), BinaryCount(error, &vm->mem)), vm);
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

static Result VMIsFloat(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsFloat(value)));
}

static Result VMIsInt(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsInt(value)));
}

static Result VMIsSym(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsSym(value)));
}

static Result VMIsPair(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsPair(value)));
}

static Result VMIsTuple(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsTuple(value, &vm->mem)));
}

static Result VMIsBin(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsBinary(value, &vm->mem)));
}

static Result VMIsMap(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsMap(value, &vm->mem)));
}

static Result VMIsFunc(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackPop(vm);
  return OkResult(BoolVal(IsFunc(value, &vm->mem)));
}

static Result VMNotEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(!ValEqual(b, a, &vm->mem)));
}

static Result VMLessEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(RawNum(b) <= RawNum(a)));
}

static Result VMGreaterEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(RawNum(b) >= RawNum(a)));
}

static Result VMNot(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  return OkResult(BoolVal(arg == False || arg == Nil));
}

static Result VMSub(u32 num_args, VM *vm)
{
  if (num_args == 1) {
    Val arg = StackPop(vm);
    if (IsInt(arg)) {
      return OkResult(IntVal(-RawInt(arg)));
    } else if (IsFloat(arg)) {
      return OkResult(FloatVal(-RawFloat(arg)));
    } else {
      return RuntimeError("Negative is only defined for numbers", vm);
    }
  } else if (num_args == 2) {
    Val a = StackPop(vm);
    Val b = StackPop(vm);
    if (!IsNum(b, &vm->mem) || !IsNum(a, &vm->mem)) {
      return RuntimeError("Subtraction is only defined for numbers", vm);
    } else if (IsInt(b) && IsInt(a)) {
      return OkResult(IntVal(RawInt(b) - RawInt(a)));
    } else {
      return OkResult(FloatVal(RawNum(b) - RawNum(a)));
    }
  } else {
    return RuntimeError("Wrong number of arguments", vm);
  }
}

static Result VMAdd(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  if (IsInt(b) && IsInt(a)) {
    return OkResult(IntVal(RawInt(b) + RawInt(a)));
  } else {
    return OkResult(FloatVal(RawNum(b) + RawNum(a)));
  }
}

static Result VMBitNot(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  return OkResult(IntVal(~RawInt(arg)));
}

static Result VMLen(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackPop(vm);
  if (IsPair(arg)) {
    return OkResult(IntVal(ListLength(arg, &vm->mem)));
  } else if (IsTuple(arg, &vm->mem)) {
    return OkResult(IntVal(TupleCount(arg, &vm->mem)));
  } else if (IsBinary(arg, &vm->mem)) {
    return OkResult(IntVal(BinaryCount(arg, &vm->mem)));
  } else if (IsMap(arg, &vm->mem)) {
    return OkResult(IntVal(MapCount(arg, &vm->mem)));
  } else {
    return RuntimeError("Length is only defined for collections", vm);
  }
}

static Result VMMul(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  if (IsInt(b) && IsInt(a)) {
    return OkResult(IntVal(RawInt(b) * RawInt(a)));
  } else {
    return OkResult(FloatVal(RawNum(b) * RawNum(a)));
  }
}

static Result VMDiv(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  if (RawNum(a) == 0) {
    return RuntimeError("Division by zero", vm);
  } else {
    return OkResult(FloatVal(RawNum(b) / RawNum(a)));
  }
}

static Result VMRem(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);
  return OkResult(IntVal(RawInt(b) % RawInt(a)));
}

static Result VMShiftLeft(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(IntVal(RawInt(b) << RawInt(a)));
}

static Result VMShiftRight(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(IntVal(RawInt(b) >> RawInt(a)));
}

static Result VMBitAnd(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(IntVal(RawInt(b) & RawInt(a)));
}

static Result VMBitOr(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(IntVal(RawInt(b) | RawInt(a)));
}

static Result VMIn(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  if (IsPair(a)) {
    return OkResult(BoolVal(ListContains(a, b, &vm->mem)));
  } else if (IsTuple(a, &vm->mem)) {
    return OkResult(BoolVal(TupleContains(a, b, &vm->mem)));
  } else if (IsBinary(a, &vm->mem)) {
    return OkResult(BoolVal(BinaryContains(a, b, &vm->mem)));
  } else if (IsMap(a, &vm->mem)) {
    return OkResult(BoolVal(MapContains(a, b, &vm->mem)));
  } else {
    return OkResult(False);
  }
}

static Result VMLess(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(RawNum(b) < RawNum(a)));
}

static Result VMGreater(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(RawNum(b) > RawNum(a)));
}

static Result VMEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(BoolVal(ValEqual(b, a, &vm->mem)));
}

static Result VMPair(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackPop(vm);
  b = StackPop(vm);

  return OkResult(Pair(a, b, &vm->mem));
}

static Result VMCat(u32 num_args, VM *vm)
{
  Val concatenated;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  if (IsPair(StackRef(vm, 0)) && IsPair(StackRef(vm, 1))) {
    concatenated = ListCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsTuple(StackRef(vm, 0), &vm->mem) && IsTuple(StackRef(vm, 1), &vm->mem)) {
    concatenated = TupleCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsBinary(StackRef(vm, 0), &vm->mem) && IsBinary(StackRef(vm, 1), &vm->mem)) {
    concatenated = BinaryCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsMap(StackRef(vm, 0), &vm->mem) && IsMap(StackRef(vm, 1), &vm->mem)) {
    if (MapCount(StackRef(vm, 1), &vm->mem) == 0) {
      concatenated = StackRef(vm, 0);
    } else if (MapCount(StackRef(vm, 0), &vm->mem) == 0) {
      concatenated = StackRef(vm, 1);
    } else {
      u32 i;
      u32 count = MapCount(StackRef(vm, 0), &vm->mem);
      for (i = 0; i < count; i++) {
        Val key = MapGetKey(StackRef(vm, 0), i, &vm->mem);
        Val value = MapGet(StackRef(vm, 0), key, &vm->mem);
        MapSet(StackRef(vm, 1), key, value, &vm->mem);
      }
      concatenated = StackRef(vm, 1);
    }
  } else {
    return RuntimeError("Concatenation is only defined for collections of the same type", vm);
  }

  StackPop(vm);
  StackPop(vm);
  return OkResult(concatenated);
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

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
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
  Val types[2] = {AnyType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  length = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
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
  Val types[2] = {AnyType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  data = StackPop(vm);
  id = RawInt(StackPop(vm));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
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

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
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
  Val types[3] = {AnyType, SymType, IntType};
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

static Result VMMapGet(u32 num_args, VM *vm)
{
  Val key, map;
  Val types[2] = {AnyType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackPop(vm);
  map = StackPop(vm);
  return OkResult(MapGet(map, key, &vm->mem));
}

static Result VMMapSet(u32 num_args, VM *vm)
{
  Val key, value, map;
  Val types[3] = {AnyType, AnyType, MapType};
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
  Val types[2] = {AnyType, MapType};
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
  } else if ((u32)RawInt(index) >= BinaryCount(bin, &vm->mem)) {
    return OkResult(Pair(bin, MakeBinary(0, &vm->mem), &vm->mem));
  } else {
    u32 tail_len = BinaryCount(bin, &vm->mem) - RawInt(index);
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
    return OkResult(BinaryCount(iodata, &vm->mem));
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
    Copy(BinaryData(iodata, mem), buf+index, BinaryCount(iodata, mem));
    return BinaryCount(iodata, mem);
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
  Val types[1] = {AnyType};
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
  for (i = 0; i < BinaryCount(bin, &vm->mem); i++) {
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
