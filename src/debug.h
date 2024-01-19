#include "compile/env.h"
#include "compile/parse.h"
#include "runtime/chunk.h"
#include "runtime/mem/symbols.h"
#include "runtime/ops.h"
#include "runtime/vm.h"

u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols);
void PrintMem(Mem *mem, SymbolTable *symbols);
void DumpMem(char *filename, Mem *mem, SymbolTable *symbols);

void PrintAST(Node *ast, SymbolTable *symbols);
void PrintCompileEnv(Frame *frame, SymbolTable *symbols);

void Disassemble(Chunk *chunk);
void DumpSourceMap(Chunk *chunk);

void PrintTraceHeader(u32 chunk_size);
void TraceInstruction(VM *vm);
void PrintEnv(Val env, Mem *mem, SymbolTable *symbols);
