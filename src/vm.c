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
