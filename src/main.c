#include "chunk.h"
#include "compile.h"
#include "vm.h"
#include "run.h"
#include "test.h"

int main(int argc, char *argv[])
{
  Chunk chunk;
  InitChunk(&chunk);

  WriteChunk(&chunk, 7,
    OP_CONST, IntVal(1),
    OP_CONST, IntVal(2),
    OP_CONST, IntVal(3),
    OP_MUL,
    OP_ADD,
    OP_CONST, IntVal(4),
    OP_SUB);
  TestCompile("1 + 2 * 3 - 4", &chunk);
  ResetChunk(&chunk);

  WriteChunk(&chunk, 14,
    OP_CONST, IntVal(1),
    OP_CONST, IntVal(2),
    OP_CONST, IntVal(1),
    OP_SUB,
    OP_EQUAL,
    OP_DUP,
    OP_BRANCH, 2,
    OP_POP,
    OP_NIL,
    OP_NOT,
    OP_BRANCH, 4,
    OP_CONST, SymbolFor("ok"),
    OP_JUMP, 2,
    OP_CONST, SymbolFor("err"));
  TestCompile("if 1 = 2 - 1 or nil :ok :err", &chunk);
  ResetChunk(&chunk);

  WriteChunk(&chunk, 37,
    OP_CONST, MakeSymbol(&chunk.symbols, "foo"),
    OP_CONST, MakeSymbol(&chunk.symbols, "x"),
    OP_CONST, MakeSymbol(&chunk.symbols, "sum"),
    OP_JUMP, 33,
    OP_CONST, SymbolFor("x"),
    OP_LOOKUP,
    OP_ZERO,
    OP_EQUAL,
    OP_NOT,
    OP_BRANCH, 5,
    OP_CONST, SymbolFor("sum"),
    OP_LOOKUP,
    OP_JUMP, 19,
    OP_CONST, SymbolFor("x"),
    OP_LOOKUP,
    OP_CONST, IntVal(1),
    OP_SUB,
    OP_CALL,
    OP_CONST, SymbolFor("sum"),
    OP_LOOKUP,
    OP_CONST, SymbolFor("x"),
    OP_LOOKUP,
    OP_ADD,
    OP_CALL,
    OP_CONST, SymbolFor("foo"),
    OP_LOOKUP,
    OP_APPLY,
    OP_RETURN,
    OP_CONST, IntVal(8),
    OP_LAMBDA, 2,
    OP_DEFINE,
    OP_POP,
    OP_CONST, IntVal(3),
    OP_ZERO,
    OP_CONST, SymbolFor("foo"),
    OP_LOOKUP,
    OP_CALL);
  TestCompile("def foo (x sum) -> if x = 0 sum (foo (x - 1) (sum + x)), foo 3 0", &chunk);
  ResetChunk(&chunk);

  WriteChunk(&chunk, 5,
    OP_CONST, PutSymbol(&chunk, "x", 1),
    OP_CONST, IntVal(1),
    OP_CONST, PutSymbol(&chunk, "y", 1),
    OP_CONST, IntVal(2),
    OP_DICT, 2);
  TestCompile("{x: 1, y: 2}", &chunk);
  ResetChunk(&chunk);

  printf("\n");
}
