#include "chunk.h"
#include "vm.h"

/*
  nil
  const foo
  define
  jump <after>
f:
  const x
  lookup
  zero
  equal
  branch <done>
  const x
  lookup
  const 1
  sub
  const x
  lookup
  const sum
  lookup
  add
  const foo
  lookup
  apply
  return
done:
  const sum
  lookup
  return
after:
  const <f>
  const x
  const sum
  lambda 2
  const foo
  define
  pop

  const 3
  const 0
  const foo
  lookup
  apply
  break
*/

int main(int argc, char *argv[])
{
  Chunk chunk;
  InitChunk(&chunk);

  // declare foo in env
  PutInst(&chunk, OP_NIL);
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "foo"));
  PutInst(&chunk, OP_DEFINE);
  PutInst(&chunk, OP_POP);

  // skip function body
  PutInst(&chunk, OP_JUMP, 0);

  // function body
  u32 start = ChunkSize(&chunk);

  // if x = 0
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "x"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_ZERO);
  PutInst(&chunk, OP_EQUAL);
  PutInst(&chunk, OP_BRANCH, 0);
  u32 branch = ChunkSize(&chunk);
  // false branch
  // x - 1
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "x"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_CONST, IntVal(1));
  PutInst(&chunk, OP_SUB);
  // x + sum
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "x"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "sum"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_ADD);
  // recursively call foo
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "foo"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_APPLY);
  PutInst(&chunk, OP_RETURN);
  SetByte(&chunk, branch - 1, ChunkSize(&chunk) - branch);
  // true branch
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "sum"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_RETURN);

  // patch jump to here
  SetByte(&chunk, start - 1, ChunkSize(&chunk) - start);

  // create lambda (position, params)
  PutInst(&chunk, OP_CONST, IntVal(start));
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "x"));
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "sum"));
  PutInst(&chunk, OP_LAMBDA, 2);

  // define lambda as f
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "foo"));
  PutInst(&chunk, OP_DEFINE);
  PutInst(&chunk, OP_POP);

  // call f 3 0
  PutInst(&chunk, OP_CONST, IntVal(3));
  PutInst(&chunk, OP_ZERO);
  PutInst(&chunk, OP_CONST, PutSymbol(&chunk, "foo"));
  PutInst(&chunk, OP_LOOKUP);
  PutInst(&chunk, OP_APPLY);

  PutInst(&chunk, OP_BREAK);
  Disassemble("Chunk", &chunk);

  VM vm;
  InitVM(&vm);

  RunChunk(&vm, &chunk);

  ResetChunk(&chunk);
  ResetVM(&vm);
}
