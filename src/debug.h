#ifdef DEBUG

#include "mem/mem.h"
#include "mem/symbols.h"
#include "result.h"
#include "runtime/ops.h"
#include "runtime/chunk.h"
#include "runtime/vm.h"

void DumpMem(Mem *mem, SymbolTable *symbols);
void PrintError(Result error);
void PrintEnv(Val env, Mem *mem, SymbolTable *symbols);
void Disassemble(Chunk *chunk);
void PrintTraceHeader(void);
void TraceInstruction(OpCode op, VM *vm);
void GenerateSymbols(void);

#endif
