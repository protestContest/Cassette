#include "vm.h"

void InitVM(VM *vm)
{
  vm->chunk = NULL;
  InitMem(&vm->mem);
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

u32 OpLength(OpCode op)
{
  switch (op)
  {
  case OpConst:   return 2;
  case OpStr:     return 1;
  case OpPair:    return 1;
  case OpList:    return 2;
  case OpTuple:   return 2;
  case OpMap:     return 2;
  case OpTrue:    return 1;
  case OpFalse:   return 1;
  case OpNil:     return 1;
  case OpAdd:     return 1;
  case OpSub:     return 1;
  case OpMul:     return 1;
  case OpDiv:     return 1;
  case OpNeg:     return 1;
  case OpNot:     return 1;
  case OpEq:      return 1;
  case OpGt:      return 1;
  case OpLt:      return 1;
  case OpIn:      return 1;
  case OpAccess:  return 1;
  case OpLambda:  return 2;
  case OpApply:   return 2;
  case OpReturn:  return 1;
  case OpLookup:  return 1;
  case OpDefine:  return 1;
  case OpJump:    return 2;
  case OpBranch:  return 2;
  case OpBranchF: return 2;
  case OpPop:     return 1;
  case OpHalt:    return 1;
  }
}

u32 PrintInstruction(Chunk *chunk, u32 index)
{
  switch (chunk->data[index]) {
  case OpConst:
    Print("const ");
    return DebugVal(ChunkConst(chunk, index+1), &chunk->constants) + 6;
  case OpStr:
    return Print("str");
  case OpPair:
    return Print("pair");
  case OpList:
    Print("list ");
    return DebugVal(ChunkConst(chunk, index+1), &chunk->constants) + 5;
  case OpTuple:
    Print("tuple ");
    return DebugVal(ChunkConst(chunk, index+1), &chunk->constants) + 6;
  case OpMap:
    Print("map ");
    return DebugVal(ChunkConst(chunk, index+1), &chunk->constants) + 4;
  case OpTrue:
    return Print("true");
  case OpFalse:
    return Print("false");
  case OpNil:
    return Print("nil");
  case OpAdd:
    return Print("add");
  case OpSub:
    return Print("sub");
  case OpMul:
    return Print("mul");
  case OpDiv:
    return Print("div");
  case OpNeg:
    return Print("neg");
  case OpNot:
    return Print("not");
  case OpEq:
    return Print("eq");
  case OpGt:
    return Print("gt");
  case OpLt:
    return Print("lt");
  case OpIn:
    return Print("in");
  case OpAccess:
    return Print("access");
  case OpLambda:
    Print("lambda ");
    return PrintInt(RawInt(ChunkConst(chunk, index+1))) + 7;
  case OpApply:
    Print("apply ");
    return PrintInt(ChunkRef(chunk, index+1)) + 6;
  case OpReturn:
    return Print("return");
  case OpLookup:
    return Print("lookup");
  case OpDefine:
    return Print("define");
  case OpJump:
    Print("jump ");
    return PrintInt(RawInt(ChunkConst(chunk, index+1))) + 5;
  case OpBranch:
    Print("branch ");
    return PrintInt(RawInt(ChunkConst(chunk, index+1))) + 6;
  case OpBranchF:
    Print("branchf ");
    return PrintInt(RawInt(ChunkConst(chunk, index+1))) + 7;
  case OpPop:
    return Print("pop");
  case OpHalt:
    return Print("halt");
  default:
    Print("? ");
    return PrintInt(ChunkRef(chunk, index)) + 2;
  }
}

void TraceInstruction(VM *vm)
{
  u32 written = PrintInstruction(vm->chunk, vm->pc);
  Assert(written < 20);
  for (u32 i = 0; i < 20 - written; i++) Print(" ");
  for (u32 i = 0; i < VecCount(vm->stack) && i < 8; i++) {
    Print(" | ");
    DebugVal(vm->stack[i], &vm->mem);
  }
  Print("\n");
}

void RunChunk(VM *vm, Chunk *chunk)
{
  vm->chunk = chunk;
  vm->pc = 0;
  vm->stack = NULL;
  MergeStrings(&vm->mem, &chunk->constants);

  Mem *mem = &vm->mem;

  while (vm->pc < VecCount(chunk->data)) {
    TraceInstruction(vm);

    switch (ChunkRef(chunk, vm->pc)) {
    case OpConst:
      StackPush(vm, ChunkConst(chunk, vm->pc+1));
      vm->pc += 2;
      break;
    case OpStr:
      StackPush(vm, MakeBinary(SymbolName(StackPop(vm), mem), mem));
      vm->pc++;
      break;
    case OpPair: {
      Val tail = StackPop(vm);
      StackPush(vm, Pair(StackPop(vm), tail, &vm->mem));
      vm->pc++;
      break;
    }
    case OpList: {
      Val list = nil;
      for (u32 i = 0; i < RawInt(ChunkConst(chunk, vm->pc+1)); i++) {
        list = Pair(StackPop(vm), list, mem);
      }
      StackPush(vm, list);
      vm->pc += 2;
      break;
    }
    case OpTuple: {
      u32 length = RawInt(ChunkConst(chunk, vm->pc+1));
      Val tuple = MakeTuple(length, mem);
      for (u32 i = 0; i < length; i++) {
        TupleSet(tuple, i, StackRef(vm, length - i - 1), mem);
      }
      RewindVec(vm->stack, length);
      StackPush(vm, tuple);
      vm->pc += 2;
      break;
    }
    case OpMap:
      RuntimeError(vm, "Unimplemented");
      vm->pc += 2;
      break;
    case OpTrue:
      StackPush(vm, SymbolFor("true"));
      vm->pc++;
      break;
    case OpFalse:
      StackPush(vm, SymbolFor("false"));
      vm->pc++;
      break;
    case OpNil:
      StackPush(vm, nil);
      vm->pc++;
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
      vm->pc++;
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
      vm->pc++;
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
      vm->pc++;
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
      vm->pc++;
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
      vm->pc++;
      break;
    case OpNot:
      if (IsTrue(StackPop(vm))) {
        StackPush(vm, SymbolFor("false"));
      } else {
        StackPush(vm, SymbolFor("true"));
      }
      vm->pc++;
      break;
    case OpEq:
      StackPush(vm, BoolVal(Eq(StackPop(vm), StackPop(vm))));
      vm->pc++;
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
      vm->pc++;
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
      vm->pc++;
      break;
    case OpIn:
      RuntimeError(vm, "Unimplemented");
      vm->pc++;
      break;
    case OpAccess:
      RuntimeError(vm, "Unimplemented");
      vm->pc++;
      break;
    case OpLambda:
      RuntimeError(vm, "Unimplemented");
      vm->pc += 2;
      break;
    case OpApply:
      RuntimeError(vm, "Unimplemented");
      vm->pc += 2;
      break;
    case OpReturn:
      RuntimeError(vm, "Unimplemented");
      break;
    case OpLookup:
      RuntimeError(vm, "Unimplemented");
      vm->pc++;
      break;
    case OpDefine:
      RuntimeError(vm, "Unimplemented");
      vm->pc++;
      break;
    case OpJump:
      vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      break;
    case OpBranch:
      if (IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += 2;
      }
      break;
    case OpBranchF:
      if (!IsTrue(StackRef(vm, 0))) {
        vm->pc = RawInt(ChunkConst(chunk, vm->pc+1));
      } else {
        vm->pc += 2;
      }
      break;
    case OpPop:
      RewindVec(vm->stack, 1);
      vm->pc++;
      break;
    case OpHalt:
      vm->pc = VecCount(chunk->data);
      break;
    }
  }
}
