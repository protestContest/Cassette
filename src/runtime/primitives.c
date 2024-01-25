#include "primitives.h"
#include "env.h"
#include "app/canvas.h"
#include "compile/parse.h"
#include "compile/project.h"
#include "device/device.h"
#include "univ/hash.h"
#include "univ/math.h"
#include "univ/str.h"
#include "univ/system.h"

#define FloatType         0x7FDF2D07 /* float */
#define NumType           0x7FDFEE5C /* number */
#define AnyType           0x7FD87D24 /* any */

static Result VMHead(u32 num_args, VM *vm);
static Result VMTail(u32 num_args, VM *vm);
static Result VMPanic(u32 num_args, VM *vm);
static Result VMExit(u32 num_args, VM *vm);
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

static Result VMLen(u32 num_args, VM *vm);
static Result VMNot(u32 num_args, VM *vm);
static Result VMBitNot(u32 num_args, VM *vm);
static Result VMMul(u32 num_args, VM *vm);
static Result VMDiv(u32 num_args, VM *vm);
static Result VMRem(u32 num_args, VM *vm);
static Result VMAdd(u32 num_args, VM *vm);
static Result VMSub(u32 num_args, VM *vm);
static Result VMShiftLeft(u32 num_args, VM *vm);
static Result VMShiftRight(u32 num_args, VM *vm);
static Result VMBitAnd(u32 num_args, VM *vm);
static Result VMBitOr(u32 num_args, VM *vm);
static Result VMPair(u32 num_args, VM *vm);
static Result VMRange(u32 num_args, VM *vm);
static Result VMCat(u32 num_args, VM *vm);
static Result VMIn(u32 num_args, VM *vm);
static Result VMLess(u32 num_args, VM *vm);
static Result VMGreater(u32 num_args, VM *vm);
static Result VMLessEqual(u32 num_args, VM *vm);
static Result VMGreaterEqual(u32 num_args, VM *vm);
static Result VMEqual(u32 num_args, VM *vm);
static Result VMNotEqual(u32 num_args, VM *vm);

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
static Result VMSubStr(u32 num_args, VM *vm);
static Result VMStuff(u32 num_args, VM *vm);
static Result VMTrunc(u32 num_args, VM *vm);
static Result VMSymName(u32 num_args, VM *vm);
static Result VMDeviceList(u32 num_args, VM *vm);

static PrimitiveDef primitives[] = {
  {"head",            0x7FD4FAFD, &VMHead},
  {"tail",            0x7FD1655A, &VMTail},
  {"panic!",          0x7FDA4AE9, &VMPanic},
  {"exit",            0x7FD1CB34, &VMExit},
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
  {"..",              0x7FD62EE1, &VMRange},
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
  {"substr",          0x7FD808FD, &VMSubStr},
  {"stuff",           0x7FD2CC7F, &VMStuff},
  {"trunc",           0x7FD36865, &VMTrunc},
  {"symbol-name",     0x7FD0CEDC, &VMSymName},
  {"device-list",     0x7FD44F43, &VMDeviceList}
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

Result Access(Val val, Val index, VM *vm)
{
  if (IsPair(val)) {
    if (!IsInt(index)) {
      return RuntimeError("Index for a list must be an integer", vm);
    }
    if (RawInt(index) < 0) return RuntimeError("Index out of bounds", vm);
    return ValueResult(ListGet(val, RawInt(index), &vm->mem));
  } else if (IsTuple(val, &vm->mem)) {
    if (!IsInt(index)) {
      return RuntimeError("Index for a tuple must be an integer", vm);
    }
    if (RawInt(index) < 0 || (u32)RawInt(index) >= TupleCount(val, &vm->mem)) {
      return RuntimeError("Index out of bounds", vm);
    }
    return ValueResult(TupleGet(val, RawInt(index), &vm->mem));
  } else if (IsBinary(val, &vm->mem)) {
    if (!IsInt(index)) {
      return RuntimeError("Index for a binary must be an integer", vm);
    }
    if (RawInt(index) < 0 || (u32)RawInt(index) >= BinaryCount(val, &vm->mem)) {
      return RuntimeError("Index out of bounds", vm);
    }
    return ValueResult(IntVal(BinaryData(val, &vm->mem)[RawInt(index)]));
  } else if (IsMap(val, &vm->mem)) {
    if (!MapContains(val, index, &vm->mem)) {
      return RuntimeError("Undefined map key", vm);
    }
    return ValueResult(MapGet(val, index, &vm->mem));
  } else {
    return RuntimeError("Tried to access a non-collection", vm);
  }
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
  if (num_args != num_params) {
    return RuntimeError("Wrong number of arguments", vm);
  }
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
  return ValueResult(Nil);
}

static Result OkError(Result result, Mem *mem)
{
  char *message = ResultError(result)->message;
  return ValueResult(
    Pair(Error, BinaryFrom(message, StrLen(message), mem), mem));
}

static Result VMHead(u32 num_args, VM *vm)
{
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  return ValueResult(Head(StackRef(vm, 0), &vm->mem));
}

static Result VMTail(u32 num_args, VM *vm)
{
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  return ValueResult(Tail(StackRef(vm, 0), &vm->mem));
}

static Result VMPanic(u32 num_args, VM *vm)
{
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  if (IsBinary(StackRef(vm, 0), &vm->mem)) {
    return RuntimeError(CopyBin(StackRef(vm, 0), &vm->mem), vm);
  } else {
    return RuntimeError("Unknown error", vm);
  }
}

static Result VMExit(u32 num_args, VM *vm)
{
  Halt(vm);
  return ValueResult(Nil);
}

static Result VMUnwrap(u32 num_args, VM *vm)
{
  Val value, default_val;
  Val types[2] = {PairType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  default_val = StackRef(vm, 0);
  value = StackRef(vm, 1);
  if (Head(value, &vm->mem) == Ok) {
    return ValueResult(Tail(value, &vm->mem));
  } else {
    return ValueResult(default_val);
  }
}

static Result VMForceUnwrap(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {PairType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  if (Head(value, &vm->mem) == Ok) {
    return ValueResult(Tail(value, &vm->mem));
  } else {
    Val error = Tail(value, &vm->mem);
    if (IsBinary(error, &vm->mem)) {
      return RuntimeError(CopyBin(error, &vm->mem), vm);
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

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(Head(value, &vm->mem) == Ok));
}

static Result VMIsFloat(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsFloat(value)));
}

static Result VMIsInt(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsInt(value)));
}

static Result VMIsSym(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsSym(value)));
}

static Result VMIsPair(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsPair(value)));
}

static Result VMIsTuple(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsTuple(value, &vm->mem)));
}

static Result VMIsBin(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsBinary(value, &vm->mem)));
}

static Result VMIsMap(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsMap(value, &vm->mem)));
}

static Result VMIsFunc(u32 num_args, VM *vm)
{
  Val value;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  return ValueResult(BoolVal(IsFunc(value, &vm->mem)));
}

static Result VMLen(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackRef(vm, 0);
  if (IsPair(arg)) {
    return ValueResult(IntVal(ListLength(arg, &vm->mem)));
  } else if (IsTuple(arg, &vm->mem)) {
    return ValueResult(IntVal(TupleCount(arg, &vm->mem)));
  } else if (IsBinary(arg, &vm->mem)) {
    return ValueResult(IntVal(BinaryCount(arg, &vm->mem)));
  } else if (IsMap(arg, &vm->mem)) {
    return ValueResult(IntVal(MapCount(arg, &vm->mem)));
  } else {
    return RuntimeError("Length is only defined for collections", vm);
  }
}

static Result VMNot(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackRef(vm, 0);
  return ValueResult(BoolVal(arg == False || arg == Nil));
}

static Result VMBitNot(u32 num_args, VM *vm)
{
  Val arg;
  Val types[1] = {IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  arg = StackRef(vm, 0);
  return ValueResult(IntVal(~RawInt(arg)));
}

static Result VMMul(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  if (IsInt(b) && IsInt(a)) {
    return ValueResult(IntVal(RawInt(b) * RawInt(a)));
  } else {
    return ValueResult(FloatVal(RawNum(b) * RawNum(a)));
  }
}

static Result VMDiv(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  if (RawNum(a) == 0) {
    return RuntimeError("Division by zero", vm);
  } else {
    return ValueResult(FloatVal(RawNum(b) / RawNum(a)));
  }
}

static Result VMRem(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);
  return ValueResult(IntVal(RawInt(b) % RawInt(a)));
}

static Result VMAdd(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  if (IsInt(b) && IsInt(a)) {
    return ValueResult(IntVal(RawInt(b) + RawInt(a)));
  } else {
    return ValueResult(FloatVal(RawNum(b) + RawNum(a)));
  }
}

static Result VMSub(u32 num_args, VM *vm)
{
  if (num_args == 1) {
    Val arg = StackRef(vm, 0);
    if (IsInt(arg)) {
      return ValueResult(IntVal(-RawInt(arg)));
    } else if (IsFloat(arg)) {
      return ValueResult(FloatVal(-RawFloat(arg)));
    } else {
      return RuntimeError("Negative is only defined for numbers", vm);
    }
  } else if (num_args == 2) {
    Val a = StackRef(vm, 0);
    Val b = StackRef(vm, 1);
    if (!IsNum(b, &vm->mem) || !IsNum(a, &vm->mem)) {
      return RuntimeError("Subtraction is only defined for numbers", vm);
    } else if (IsInt(b) && IsInt(a)) {
      return ValueResult(IntVal(RawInt(b) - RawInt(a)));
    } else {
      return ValueResult(FloatVal(RawNum(b) - RawNum(a)));
    }
  } else {
    return RuntimeError("Wrong number of arguments", vm);
  }
}

static Result VMShiftLeft(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(IntVal(RawInt(b) << RawInt(a)));
}

static Result VMShiftRight(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(IntVal(RawInt(b) >> RawInt(a)));
}

static Result VMBitAnd(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(IntVal(RawInt(b) & RawInt(a)));
}

static Result VMBitOr(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(IntVal(RawInt(b) | RawInt(a)));
}

static Result VMPair(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(Pair(a, b, &vm->mem));
}

static Result VMRange(u32 num_args, VM *vm)
{
  i32 a, b, i;
  Val list = Nil;
  Val types[2] = {IntType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  b = RawInt(StackRef(vm, 0));
  a = RawInt(StackRef(vm, 1));

  if (a > b) return ValueResult(Nil);

  for (i = b - 1; i >= a; i--) {
    list = Pair(IntVal(i), list, &vm->mem);
  }

  return ValueResult(list);
}

static Result VMCat(u32 num_args, VM *vm)
{
  Val concatenated;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  if (IsPair(StackRef(vm, 0)) && IsPair(StackRef(vm, 1))) {
    concatenated = ListCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsTuple(StackRef(vm, 0), &vm->mem) &&
             IsTuple(StackRef(vm, 1), &vm->mem)) {
    concatenated = TupleCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsBinary(StackRef(vm, 0), &vm->mem) &&
             IsBinary(StackRef(vm, 1), &vm->mem)) {
    concatenated = BinaryCat(StackRef(vm, 1), StackRef(vm, 0), &vm->mem);
  } else if (IsMap(StackRef(vm, 0), &vm->mem) &&
             IsMap(StackRef(vm, 1), &vm->mem)) {
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

  return ValueResult(concatenated);
}

static Result VMIn(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  if (IsPair(a)) {
    return ValueResult(BoolVal(ListContains(a, b, &vm->mem)));
  } else if (IsTuple(a, &vm->mem)) {
    return ValueResult(BoolVal(TupleContains(a, b, &vm->mem)));
  } else if (IsBinary(a, &vm->mem)) {
    return ValueResult(BoolVal(BinaryContains(a, b, &vm->mem)));
  } else if (IsMap(a, &vm->mem)) {
    return ValueResult(BoolVal(MapContains(a, b, &vm->mem)));
  } else {
    return ValueResult(False);
  }
}

static Result VMLess(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(RawNum(b) < RawNum(a)));
}

static Result VMGreater(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(RawNum(b) > RawNum(a)));
}

static Result VMLessEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(RawNum(b) <= RawNum(a)));
}

static Result VMGreaterEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {NumType, NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(RawNum(b) >= RawNum(a)));
}

static Result VMEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(ValEqual(b, a, &vm->mem)));
}

static Result VMNotEqual(u32 num_args, VM *vm)
{
  Val a, b;
  Val types[2] = {AnyType, AnyType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  a = StackRef(vm, 0);
  b = StackRef(vm, 1);

  return ValueResult(BoolVal(!ValEqual(b, a, &vm->mem)));
}

static Result VMOpen(u32 num_args, VM *vm)
{
  Val opts;
  DeviceType type;
  u32 id;
  Val types[2] = {TupleType, SymType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  opts = StackRef(vm, 0);
  type = GetDeviceType(StackRef(vm, 1));
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
  vm->devices[id].context = ResultItem(result);

  return ValueResult(Pair(Ok, IntVal(id), &vm->mem));
}

static Result VMClose(u32 num_args, VM *vm)
{
  u32 id;
  Val types[1] = {IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  id = RawInt(StackRef(vm, 0));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceClose(&vm->devices[id], &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  vm->dev_map &= ~Bit(id);

  return ValueResult(Nil);
}

static Result VMRead(u32 num_args, VM *vm)
{
  u32 id;
  Val length;
  Val types[2] = {AnyType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  length = StackRef(vm, 0);
  id = RawInt(StackRef(vm, 1));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceRead(&vm->devices[id], length, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return ValueResult(Pair(Ok, ResultValue(result), &vm->mem));
}

static Result VMWrite(u32 num_args, VM *vm)
{
  u32 id;
  Val data;
  Val types[2] = {AnyType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  data = StackRef(vm, 0);
  id = RawInt(StackRef(vm, 1));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceWrite(&vm->devices[id], data, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return ValueResult(Pair(Ok, ResultValue(result), &vm->mem));
}

static Result VMGetParam(u32 num_args, VM *vm)
{
  u32 id;
  Val key;
  Val types[2] = {SymType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackRef(vm, 0);
  id = RawInt(StackRef(vm, 1));

  if (id > ArrayCount(vm->devices) || (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceGet(&vm->devices[id], key, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return ValueResult(Pair(Ok, ResultValue(result), &vm->mem));
}

static Result VMSetParam(u32 num_args, VM *vm)
{
  u32 id;
  Val key, value;
  Val types[3] = {AnyType, SymType, IntType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  key = StackRef(vm, 1);
  id = RawInt(StackRef(vm, 2));

  if (id > ArrayCount(vm->devices) && (Bit(id) & vm->dev_map) == 0) {
    return OkError(ErrorResult("Not an open device", 0, 0), &vm->mem);
  }

  result = DeviceSet(&vm->devices[id], key, value, &vm->mem);
  if (!result.ok) return OkError(result, &vm->mem);

  return ValueResult(Pair(Ok, ResultValue(result), &vm->mem));
}

static Result VMMapGet(u32 num_args, VM *vm)
{
  Val key, map;
  Val types[2] = {AnyType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackRef(vm, 0);
  map = StackRef(vm, 1);
  return ValueResult(MapGet(map, key, &vm->mem));
}

static Result VMMapSet(u32 num_args, VM *vm)
{
  Val key, value, map;
  Val types[3] = {AnyType, AnyType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  value = StackRef(vm, 0);
  key = StackRef(vm, 1);
  map = StackRef(vm, 2);
  return ValueResult(MapSet(map, key, value, &vm->mem));
}

static Result VMMapDelete(u32 num_args, VM *vm)
{
  Val key, map;
  Val types[2] = {AnyType, MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  key = StackRef(vm, 0);
  map = StackRef(vm, 1);
  return ValueResult(MapDelete(map, key, &vm->mem));
}

static Result VMMapKeys(u32 num_args, VM *vm)
{
  Val map;
  Val types[1] = {MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  map = StackRef(vm, 0);
  return ValueResult(MapKeys(map, Nil, &vm->mem));
}

static Result VMMapValues(u32 num_args, VM *vm)
{
  Val map;
  Val types[1] = {MapType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  map = StackRef(vm, 0);
  return ValueResult(MapValues(map, Nil, &vm->mem));
}

static Result VMSubStr(u32 num_args, VM *vm)
{
  Val bin;
  u32 start, end;
  Val types[3] = {IntType, IntType, BinaryType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  end = RawInt(StackRef(vm, 0));
  start = RawInt(StackRef(vm, 1));
  bin = StackRef(vm, 2);

  if (start >= end || end < 0 || start >= BinaryCount(bin, &vm->mem)) {
    return ValueResult(MakeBinary(0, &vm->mem));
  } else {
    Val substr = MakeBinary(end - start, &vm->mem);
    Copy(BinaryData(bin, &vm->mem), BinaryData(substr, &vm->mem), end - start);
    return ValueResult(substr);
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

  iodata = StackRef(vm, 0);
  bin = MakeBinary(ListLength(iodata, &vm->mem), &vm->mem);
  data = BinaryData(bin, &vm->mem);
  for (i = 0; i < BinaryCount(bin, &vm->mem); i++) {
    u8 byte = RawInt(Head(iodata, &vm->mem));
    iodata = Tail(iodata, &vm->mem);
    data[i] = byte;
  }

  return ValueResult(bin);
}

static Result VMTrunc(u32 num_args, VM *vm)
{
  Val num;
  Val types[1] = {NumType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  num = StackRef(vm, 0);
  return ValueResult(IntVal((i32)RawNum(num)));
}

static Result VMSymName(u32 num_args, VM *vm)
{
  Val sym;
  char *name;
  Val types[1] = {SymType};
  Result result = CheckTypes(num_args, ArrayCount(types), types, vm);
  if (!result.ok) return result;

  sym = StackRef(vm, 0);
  name = SymbolName(sym, &vm->chunk->symbols);
  return ValueResult(BinaryFrom(name, StrLen(name), &vm->mem));
}

static Result VMDeviceList(u32 num_args, VM *vm)
{
  DeviceDriver *devices;
  u32 num_devices = GetDevices(&devices);
  u32 i;
  Val list = Nil;

  for (i = 0; i < num_devices; i++) {
    list = Pair(devices[i].name, list, &vm->mem);
  }

  return ValueResult(list);
}
