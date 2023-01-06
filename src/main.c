#include "value.h"
#include "vm.h"
#include "printer.h"

int main(void)
{
  Chunk *chunk = NewChunk();
  u8 a = PutConstant(chunk, IntVal(42));
  u8 b = PutConstant(chunk, IntVal(30));

  PutInst(chunk, OP_BRK);

  PutInst(chunk, OP_CNST, a, D0);
  PutInst(chunk, OP_CNST, b, D1);
  PutInst(chunk, OP_CMP, D1, D0);
  PutInst(chunk, OP_BLT, 5);
  PutInst(chunk, OP_SUB, D1, D0);
  PutInst(chunk, OP_BGE, -10);
  PutInst(chunk, OP_EXG, D0, D1);
  PutInst(chunk, OP_CMPI, 0, D1);
  PutInst(chunk, OP_BNE, -16);
  PutInst(chunk, OP_STOP);

  // PutInst(chunk, OP_CMPI, 0, D1);
  // PutInst(chunk, OP_BEQ, 8);

  // PutInst(chunk, OP_MOVE, D0, D2);
  // PutInst(chunk, OP_CMP, D1, D2);
  // PutInst(chunk, OP_BLE, 5);
  // PutInst(chunk, OP_SUB, D1, D2);
  // PutInst(chunk, OP_BRA, -10);

  Disassemble(chunk, "Test chunk");

  VM *vm = NewVM();
  ExecuteChunk(vm, chunk);
}
