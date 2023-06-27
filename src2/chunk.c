#include "chunk.h"
#include "vm.h"

void InitChunk(Chunk *chunk)
{
  chunk->data = NULL;
  InitMem(&chunk->constants);
}

u8 PushConst(Chunk *chunk, Val value)
{
  for (u32 i = 0; i < VecCount(chunk->constants.values); i++) {
    if (Eq(chunk->constants.values[i], value)) return i;
  }
  u32 n = VecCount(chunk->constants.values);
  VecPush(chunk->constants.values, value);
  return n;
}

void PushByte(Chunk *chunk, u8 byte)
{
  VecPush(chunk->data, byte);
}

void Disassemble(Chunk *chunk)
{
  for (u32 i = 0; i < VecCount(chunk->data); i++) {
    PrintIntN(i, 3, ' ');
    Print(" ");
    switch (chunk->data[i]) {
    case OpConst:
      Print("const ");
      PrintVal(ChunkConst(chunk, ChunkRef(chunk, i+1)), &chunk->constants);
      i++;
      break;
    case OpStr:
      Print("str");
      break;
    case OpList:
      Print("list ");
      PrintVal(ChunkConst(chunk, ChunkRef(chunk, i+1)), &chunk->constants);
      i++;
      break;
    case OpTuple:
      Print("tuple ");
      PrintVal(ChunkConst(chunk, ChunkRef(chunk, i+1)), &chunk->constants);
      i++;
      break;
    case OpMap:
      Print("map ");
      PrintVal(ChunkConst(chunk, ChunkRef(chunk, i+1)), &chunk->constants);
      i++;
      break;
    case OpTrue:
      Print("true");
      break;
    case OpFalse:
      Print("false");
      break;
    case OpNil:
      Print("nil");
      break;
    case OpAdd:
      Print("add");
      break;
    case OpSub:
      Print("sub");
      break;
    case OpMul:
      Print("mul");
      break;
    case OpDiv:
      Print("div");
      break;
    case OpNeg:
      Print("neg");
      break;
    case OpNot:
      Print("not");
      break;
    case OpEq:
      Print("eq");
      break;
    case OpGt:
      Print("gt");
      break;
    case OpLt:
      Print("lt");
      break;
    case OpIn:
      Print("in");
      break;
    case OpAccess:
      Print("access");
      break;
    case OpLambda:
      Print("lambda ");
      PrintInt(RawInt(ChunkConst(chunk, ChunkRef(chunk, i+1))));
      i++;
      break;
    case OpApply:
      Print("apply ");
      PrintInt(ChunkRef(chunk, i+1));
      i++;
      break;
    case OpReturn:
      Print("return");
      break;
    case OpLookup:
      Print("lookup");
      break;
    case OpDefine:
      Print("define");
      break;
    case OpJump:
      Print("jump ");
      PrintInt(RawInt(ChunkConst(chunk, ChunkRef(chunk, i+1))));
      i++;
      break;
    case OpBranch:
      Print("branch ");
      PrintInt(RawInt(ChunkConst(chunk, ChunkRef(chunk, i+1))));
      i++;
      break;
    case OpBranchF:
      Print("branchf ");
      PrintInt(RawInt(ChunkConst(chunk, ChunkRef(chunk, i+1))));
      i++;
      break;
    case OpPop:
      Print("pop");
      break;
    default:
      Print("? ");
      PrintInt(ChunkRef(chunk, i));
    }
    Print("\n");
  }
}
