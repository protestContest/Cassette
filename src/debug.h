#include "compile/env.h"
#include "compile/parse.h"
#include "mem/mem.h"
#include "mem/symbols.h"
#include "runtime/chunk.h"
#include "runtime/ops.h"
#include "runtime/vm.h"

u32 PrintVal(Val value, Mem *mem, SymbolTable *symbols);
void DumpMem(Mem *mem, SymbolTable *symbols);

void PrintAST(Node *ast, SymbolTable *symbols);
void PrintCompileEnv(Frame *frame, SymbolTable *symbols);

void Disassemble(Chunk *chunk);
void DumpSourceMap(Chunk *chunk);

void PrintTraceHeader(void);
void TraceInstruction(VM *vm);
void PrintEnv(Val env, Mem *mem, SymbolTable *symbols);
