#include "rec.h"

Val PrimFoo(VM *vm)
{
  Print("Foo!\n");
  return nil;
}

static PrimitiveDef primitives[] = {
  {"foo", PrimFoo}
};

int main(int argc, char *argv[])
{
  if (argc < 2) {
    Print("File required\n");
    return 1;
  }

  char *source = ReadFile(argv[1]);
  Print(source);
  Print("\n");

  Heap mem;
  InitMem(&mem, 0);

  Val ast = Parse(source, &mem);
  if (IsTagged(ast, "error", &mem)) {
    Print("Parse error\n");
    return 1;
  }

  Print("AST: ");
  Inspect(ast, &mem);
  Print("\n\n");

  SetPrimitives(primitives, ArrayCount(primitives));

  CompileResult compiled = Compile(ast, nil, &mem);
  if (!compiled.ok) {
    Print("Compile error: ");
    Print(compiled.error);
    Print("\n");
    return 1;
  }

  Print("Seq:\n");
  PrintSeq(compiled.result, &mem);

  Chunk chunk;
  InitChunk(&chunk);
  Assemble(compiled.result, &chunk, &mem);

  Disassemble(&chunk);
  Print("\n");

  DestroyMem(&mem);
  InitMem(&mem, 0);

  VM vm;
  InitVM(&vm, &mem);
  RunChunk(&vm, &chunk);

  DestroyChunk(&chunk);
  DestroyVM(&vm);
  DestroyMem(&mem);
  Free(source);
}
