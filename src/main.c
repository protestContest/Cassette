#include "value.h"
#include "vm.h"
#include "printer.h"

int main(void)
{
  Chunk *chunk = NewChunk();
  u8 a = PutConstant(chunk, IntVal(30));
  u8 b = PutConstant(chunk, IntVal(42));
  u8 zero = PutConstant(chunk, IntVal(0));
  u8 done = PutConstant(chunk, IntVal(32));
  u8 test_b = PutConstant(chunk, IntVal(9));

  Assign(chunk, a, REG_A);
  Assign(chunk, b, REG_B);
  Assign(chunk, zero, REG_Z);
// test_b
  // Break(chunk);
  Assign(chunk, done, REG_ADDR);
  Compare(chunk, REG_B, REG_Z);
  Branch(chunk, REG_ADDR);
  Rem(chunk, REG_A, REG_B, REG_T);
  Move(chunk, REG_B, REG_A);
  Move(chunk, REG_T, REG_B);
  Assign(chunk, test_b, REG_ADDR);
  Goto(chunk, REG_ADDR);
// done
  Halt(chunk);

  Disassemble(chunk);

  VM *vm = NewVM();
  ExecuteChunk(vm, chunk);
}
