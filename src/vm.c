#include "vm.h"
#include "hash.h"

/* Values are nan-boxed floats */

#define nanMask     0x7FC00000

#define type1Mask   0xFFE00000
#define symMask     0x7FC00000
#define intMask     0x7FE00000

#define type2Mask   0xFFF00000
#define pairMask    0xFFC00000
#define tupleMask   0xFFE00000
#define binMask     0xFFF00000
#define xMask       0xFFD00000

#define IsType1(v)  (((v) & 0x80000000) == 0x0)
#define NumVal(n)   (u32)(n)
#define IsNum(n)    (((n) & nanMask) != nanMask)
#define IntVal(i)   ((i) | intMask)
#define IsInt(i)    (((i) & type1Mask) == intMask)
#define SymVal(s)   ((s) | symMask)
#define IsSym(s)    (((s) & type1Mask) == symMask)
#define PairVal(p)  ((p) & pairMask)
#define IsPair(p)   (((p) & type2Mask) == pairMask)
#define TupleVal(t) ((t) & tupleMask)
#define IsTuple(t)  (((t) & type2Mask) == tupleMask)
#define BinVal(b)   ((b) & binMask)
#define IsBin(b)    (((b) & type2Mask) == binMask)

#define RawVal(v)   (IsNum(v) ? (float)(v) : IsType1(v) ? ((v) & (~type1Mask)) : ((v) & (~type2Mask)))

#define  nil_val    PairVal(0)

void InitVM(VM *vm)
{
  vm->mem[0] = nil_val;
  vm->mem[1] = nil_val;
  vm->base = 0;
  vm->free = 2;
  vm->sp = MEMORY_SIZE - 1;
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    vm->symbols[i].hash = 0;
  }
}

void StackPush(VM *vm, u32 val)
{
  if (vm->sp - 1 == vm->free) {
    Error("Stack overflow\n");
    exit(1);
  }
  vm->mem[vm->sp] = val;
  vm->sp--;
}

u32 StackPop(VM *vm)
{
  u32 val = vm->mem[vm->sp];
  vm->sp++;
  return val;
}

u32 Allocate(VM *vm, u32 size)
{
  if (vm->free + size > vm->sp) {
    Error("Can't allocate %d: Out of memory\n", size);
    DumpVM(vm);
    exit(2);
  }

  u32 index = vm->free;
  vm->free += size;
  return index;
}

u32 MakeSymbol(VM *vm, char *src)
{
  u32 len = strlen(src);
  u32 val = SymVal(Hash(src, len));
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (vm->symbols[i].hash == val) {
      return val;
    }
    if (vm->symbols[i].hash == 0) {
      vm->symbols[i].hash = val;
      vm->symbols[i].name = src;
      return val;
    }
  }
  Error("Too many symbols\n");
  exit(3);
}

u32 MakePair(VM *vm, u32 head, u32 tail)
{
  u32 index = Allocate(vm, 2);
  vm->mem[vm->base + index] = head;
  vm->mem[vm->base + index+1] = tail;
  return PairVal(index);
}

u32 Head(VM *vm, u32 pair)
{
  u32 index = RawVal(pair);
  return vm->mem[vm->base + index];
}

u32 Tail(VM *vm, u32 pair)
{
  u32 index = RawVal(pair);
  return vm->mem[vm->base + index+1];
}

void SetHead(VM *vm, u32 pair, u32 head)
{
  u32 index = RawVal(pair);
  vm->mem[vm->base + index] = head;
}

void SetTail(VM *vm, u32 pair, u32 tail)
{
  u32 index = RawVal(pair);
  vm->mem[vm->base + index+1] = tail;
}

u32 MakeTuple(VM *vm, u32 length, ...)
{
  va_list args;

  u32 index = Allocate(vm, length + 1);
  vm->mem[vm->base + index] = length;

  va_start(args, length);
  for (u32 i = 0; i < length; i++) {
    u32 arg = va_arg(args, u32);
    vm->mem[vm->base + index + 1 + i] = arg;
  }

  va_end(args);

  return TupleVal(index);
}

u32 TupleAt(VM *vm, u32 tuple, u32 i)
{
  u32 index = RawVal(tuple);
  return vm->mem[vm->base + index + 1 + i];
}

void DumpVM(VM *vm)
{
  printf("  Registers\n");

  printf("  exp:  0x%08X\n", vm->exp);
  printf("  env:  0x%08X\n", vm->env);
  printf("  proc: 0x%08X\n", vm->proc);
  printf("  argl: 0x%08X\n", vm->argl);
  printf("  val:  0x%08X\n", vm->val);
  printf("  cont: 0x%08X\n", vm->cont);
  printf("  unev: 0x%08X\n", vm->unev);
  printf("  sp:   0x%08X\n", vm->sp);
  printf("  base: 0x%08X\n", vm->base);
  printf("  free: 0x%08X\n", vm->free);
  printf("\n");

  printf("  Memory\n");
  for (u32 i = vm->base; i < vm->free; i++) {
    printf("  0x%08X    %08X\n", i, vm->mem[i]);
  }
  printf("\n");

  printf("  Stack\n");
  for (u32 i = MEMORY_SIZE - 1; i > vm->sp; i--) {
    printf("  0x%08X    %08X\n", i, vm->mem[i]);
  }
  printf("\n");

  printf("  Symbols\n");
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (vm->symbols[i].hash == 0) {
      break;
    }

    printf("  0x%08X    %s\n", vm->symbols[i].hash, vm->symbols[i].name);
  }
  printf("\n");
}
