#include "vm.h"
#include "env.h"
#include "port.h"

void InitVM(VM *vm, Mem *mem)
{
  vm->mem = mem;
  vm->stack = NULL;
  vm->env = InitialEnv(mem);
  InitMap(&vm->ports);
  vm->buffers = NULL;
  vm->windows = NULL;

  Seed(Microtime());
  OpenPort(vm, SymbolFor("console"), nil, nil);
}


void PrintReg(i32 reg)
{
  switch (reg) {
  case RegVal:  Print("[val]"); break;
  case RegExp:  Print("[exp]"); break;
  case RegEnv:  Print("[env]"); break;
  case RegCont: Print("[cont]"); break;
  case RegProc: Print("[proc]"); break;
  case RegArgs: Print("[args]"); break;
  case RegTmp:  Print("[tmp]"); break;
  default:      Print("<reg>"); break;
  }
}
