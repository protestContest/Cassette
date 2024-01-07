#include "mem/mem.h"
#include "mem/symbols.h"
#include "result.h"
#include "runtime/ops.h"
#include "runtime/chunk.h"
#include "runtime/vm.h"
#include "compile/parse.h"
#include "compile/env.h"

u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols);
void DumpMem(Mem *mem, SymbolTable *symbols);
void PrintError(Result error);
void PrintEnv(Val env, Mem *mem, SymbolTable *symbols);
void PrintCompileEnv(Frame *frame, SymbolTable *symbols);
void PrintInstruction(Chunk *chunk, u32 pos);
void Disassemble(Chunk *chunk);
void PrintTraceHeader(void);
void TraceInstruction(OpCode op, VM *vm);
void DefineSymbols(SymbolTable *symbols);
void DefinePrimitiveSymbols(SymbolTable *symbols);
void GenerateSymbols(void);
void GeneratePrimitives(void);
void PrintMemory(u32 amount);
void PrintAST(Node *ast, SymbolTable *symbols);
void DumpSourceMap(Chunk *chunk);
