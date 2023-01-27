#include "chunk.h"

int main(int argc, char *argv[])
{
  Chunk *chunk = InitChunk(NewChunk());
  PutInst(chunk, 1, OP_CONST, NumVal(1.2));
  PutInst(chunk, 1, OP_RETURN);
  Disassemble("Chunk", chunk);
}
