#include "test.h"
#include "chunk.h"
#include "compile.h"
#include "mem.h"

static u32 TestCompile(char *src, Chunk *expected)
{
  Chunk actual;
  InitChunk(&actual);
  Compile(src, &actual);

  if (!ChunksEqual(&actual, expected)) {
    printf("FAIL\n\n    %s\n\n", src);
    Disassemble("Expected", expected);
    Disassemble("Actual", &actual);
    return 0;
  } else {
    printf(".");
    return 1;
  }
}

void CompileTests(void)
{
  Chunk chunk;
  InitChunk(&chunk);

  u32 passed = 0;
  u32 total = 0;

  WriteChunk(&chunk, 7,
    OP_CONST, IntVal(1),
    OP_CONST, IntVal(2),
    OP_CONST, IntVal(3),
    OP_MUL,
    OP_ADD,
    OP_CONST, IntVal(4),
    OP_SUB);
  passed += TestCompile("1 + 2 * 3 - 4", &chunk);
  total++;
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
    OP_JUMP, 1,
    OP_NIL);
  passed += TestCompile("if 1 = 2 - 1 or nil :ok", &chunk);
  total++;
  ResetChunk(&chunk);

  WriteChunk(&chunk, 37,
    OP_CONST, MakeSymbol(&chunk.strings, "foo"),
    OP_CONST, MakeSymbol(&chunk.strings, "x"),
    OP_CONST, MakeSymbol(&chunk.strings, "sum"),
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
  passed += TestCompile("def foo (x sum) -> if x = 0 sum (foo (x - 1) (sum + x)), foo 3 0", &chunk);
  total++;
  ResetChunk(&chunk);

  WriteChunk(&chunk, 5,
    OP_CONST, PutSymbol(&chunk, "x", 1),
    OP_CONST, IntVal(1),
    OP_CONST, PutSymbol(&chunk, "y", 1),
    OP_CONST, IntVal(2),
    OP_DICT, 2);
  passed += TestCompile("{x: 1, y: 2}", &chunk);
  total++;
  ResetChunk(&chunk);

  WriteChunk(&chunk, 4,
    OP_CONST, PutSymbol(&chunk, "screen", 6),
    OP_LOOKUP,
    OP_CONST, PutSymbol(&chunk, "width", 5),
    OP_ACCESS);
  passed += TestCompile("screen.width", &chunk);
  total++;
  ResetChunk(&chunk);

  printf("\n");
  printf("Total: %d    Passed: %d\n", total, passed);
}
