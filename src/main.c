#include "vm.h"
#include "chunk.h"

int main(int argc, char *argv[])
{
  VM *vm = NewVM();
  Chunk *chunk = InitChunk(NewChunk());
  PutInst(chunk, 1, OP_CONST, NumVal(1.2));
  PutInst(chunk, 1, OP_NEG);
  PutInst(chunk, 1, OP_CONST, IntVal(33));
  PutInst(chunk, 1, OP_DIV);
  PutInst(chunk, 2, OP_RETURN);
  Disassemble("Chunk", chunk);

  Run(vm, chunk);

  FreeVM(vm);
  FreeChunk(chunk);
}
