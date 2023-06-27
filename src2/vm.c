#include "vm.h"

void InitVM(VM *vm)
{
  vm->chunk = NULL;
  InitMem(vm->mem);
  vm->stack = NULL;
  vm->pc = 0;
}

void MergeStrings(Mem *dst, Mem *src)
{
  for (u32 i = 0; i < MapCount(&src->string_map); i++) {
    u32 key = GetMapKey(&src->string_map, i);
    if (!MapContains(&dst->string_map, key)) {
      char *string = src->strings + MapGet(&src->string_map, key);
      u32 index = VecCount(dst->strings);
      while (*string) {
        VecPush(dst->strings, *string++);
      }
      VecPush(dst->strings, '\0');
      MapSet(&dst->string_map, key, index);
    }
  }
}

void StackPush(VM *vm, Val val)
{
  VecPush(vm->stack, val);
}

Val StackRef(VM *vm, u32 i)
{
  return vm->stack[VecCount(vm->stack) - 1 - i];
}

Val StackPop(VM *vm)
{
  Val value = StackRef(vm, 0);
  RewindVec(vm->stack, 1);
  return value;
}

void RuntimeError(VM *vm, char *message)
{
  Print("Runtime error: ");
  Print(message);
  Print("\n");
  vm->pc = VecCount(vm->chunk->data);
}

bool IsNumeric(Val value)
{
  return IsNum(value) || IsInt(value);
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->chunk = chunk;
  vm->pc = 0;
  vm->stack = NULL;
  MergeStrings(vm->mem, &chunk->constants);

  while (vm->pc < VecCount(chunk->data)) {
    switch (ChunkRef(chunk, vm->pc)) {
    case OpConst:
      StackPush(vm, ChunkConst(chunk, ChunkRef(chunk, vm->pc+1)));
      vm->pc += 2;
      break;
    case OpStr:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpList:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpTuple:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpMap:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpTrue:
      StackPush(vm, SymbolFor("true"));
      break;
    case OpFalse:
      StackPush(vm, SymbolFor("false"));
      break;
    case OpNil:
      StackPush(vm, nil);
      break;
    case OpAdd:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if (IsInt(a)) {
          if (IsInt(b)) {
            StackPush(vm, IntVal(RawInt(a) + RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawInt(a) + RawNum(b)));
          }
        } else {
          if (IsInt(b)) {
            StackPush(vm, NumVal(RawNum(a) + RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawNum(a) + RawNum(b)));
          }
        }
      }
      break;
    case OpSub:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if (IsInt(a)) {
          if (IsInt(b)) {
            StackPush(vm, IntVal(RawInt(a) - RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawInt(a) - RawNum(b)));
          }
        } else {
          if (IsInt(b)) {
            StackPush(vm, NumVal(RawNum(a) - RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawNum(a) - RawNum(b)));
          }
        }
      }
      break;
    case OpMul:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if (IsInt(a)) {
          if (IsInt(b)) {
            StackPush(vm, IntVal(RawInt(a) * RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawInt(a) * RawNum(b)));
          }
        } else {
          if (IsInt(b)) {
            StackPush(vm, NumVal(RawNum(a) * RawInt(b)));
          } else {
            StackPush(vm, NumVal(RawNum(a) * RawNum(b)));
          }
        }
      }
      break;
    case OpDiv:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if ((IsInt(b) && RawInt(b) == 0) || (IsNum(b) && RawNum(b) == 0)) {
          RuntimeError(vm, "Divide by zero");
        } else {
          if (IsInt(a)) {
            if (IsInt(b)) {
              StackPush(vm, NumVal((float)RawInt(a) / RawInt(b)));
            } else {
              StackPush(vm, NumVal(RawInt(a) / RawNum(b)));
            }
          } else {
            if (IsInt(b)) {
              StackPush(vm, NumVal(RawNum(a) / RawInt(b)));
            } else {
              StackPush(vm, NumVal(RawNum(a) / RawNum(b)));
            }
          }
        }
      }
      break;
    case OpNeg:
      if (!IsNumeric(StackRef(vm, 0))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val n = StackPop(vm);
        if (IsInt(n)) {
          StackPush(vm, IntVal(-RawInt(n)));
        } else {
          StackPush(vm, NumVal(-RawNum(n)));
        }
      }
      break;
    case OpNot:
      if (IsTrue(StackPop(vm))) {
        StackPush(vm, SymbolFor("false"));
      } else {
        StackPush(vm, SymbolFor("true"));
      }
      break;
    case OpEq:
      StackPush(vm, BoolVal(Eq(StackPop(vm), StackPop(vm))));
      break;
    case OpGt:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if (IsInt(a)) {
          if (IsInt(b)) {
            StackPush(vm, BoolVal(RawInt(a) > RawInt(b)));
          } else {
            StackPush(vm, BoolVal(RawInt(a) > RawNum(b)));
          }
        } else {
          if (IsInt(b)) {
            StackPush(vm, BoolVal(RawNum(a) > RawInt(b)));
          } else {
            StackPush(vm, BoolVal(RawNum(a) > RawNum(b)));
          }
        }
      }
      break;
    case OpLt:
      if (!IsNumeric(StackRef(vm, 0)) || !IsNumeric(StackRef(vm, 1))) {
        RuntimeError(vm, "Bad arithmetic argument");
      } else {
        Val b = StackPop(vm);
        Val a = StackPop(vm);
        if (IsInt(a)) {
          if (IsInt(b)) {
            StackPush(vm, BoolVal(RawInt(a) < RawInt(b)));
          } else {
            StackPush(vm, BoolVal(RawInt(a) < RawNum(b)));
          }
        } else {
          if (IsInt(b)) {
            StackPush(vm, BoolVal(RawNum(a) < RawInt(b)));
          } else {
            StackPush(vm, BoolVal(RawNum(a) < RawNum(b)));
          }
        }
      }
      break;
    case OpIn:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpAccess:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpLambda:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpApply:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpReturn:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpLookup:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpDefine:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, ChunkRef(chunk, vm->pc+1)));
      break;
    case OpBranch:
      if (IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, ChunkRef(chunk, vm->pc+1)));
      }
      break;
    case OpBranchF:
      if (!IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, ChunkRef(chunk, vm->pc+1)));
      }
      break;
    case OpPop:
      RewindVec(vm->stack, 1);
      break;
    }
  }
}
