#include "vm.h"
#include "hash.h"
#include <string.h>

/* Values are nan-boxed floats */

#define A(n)        (((n) & 0xFF000000) >> 24)
#define B(n)        (((n) & 0x00FF0000) >> 16)
#define C(n)        (((n) & 0x0000FF00) >> 8)
#define D(n)        (((n) & 0x000000FF))
#define Chr(c)      ((c) >= 0x20 && (c) < 0x7F) ? (c) : ('.')

Val nil_val = PairVal(0);
Val quote_val;

ValType TypeOf(Val v)
{
  if (IsNum(v))   return NUMBER;
  if (IsInt(v))   return INTEGER;
  if (IsSym(v))   return SYMBOL;
  if (IsPair(v))  return PAIR;
  if (IsTuple(v)) return TUPLE;
  if (IsBin(v))   return BINARY;

  Error("Unknown type");
}

void InitVM(VM *vm)
{
  vm->mem[0] = nil_val;
  vm->mem[1] = nil_val;
  vm->base = 0;
  vm->free = 2;
  vm->sp = STACK_BOTTOM;
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    vm->symbols[i].key = nil_val;
  }

  quote_val = MakeSymbol(vm, "quote", 5);
}

void StackPush(VM *vm, Val val)
{
  if (vm->sp - 1 == vm->free) {
    Error("Stack overflow");
  }
  vm->mem[vm->sp] = val;
  vm->sp--;
}

Val StackPop(VM *vm)
{
  if (vm->sp == STACK_BOTTOM) {
    Error("Stack empty");
  }
  vm->sp++;
  Val val = vm->mem[vm->sp];
  return val;
}

Val StackPeek(VM *vm, u32 i)
{
  if (vm->sp + 1 + i > STACK_BOTTOM) {
    Error("Stack underflow");
  }
  return vm->mem[vm->sp + 1 + i];
}

u32 Allocate(VM *vm, u32 size)
{
  if (vm->free + size > vm->sp) {
    DumpVM(vm);
    Error("Can't allocate %d: Out of memory", size);
  }

  u32 index = vm->free;
  vm->free += size;
  return index;
}

Val MakeSymbol(VM *vm, char *src, u32 len)
{
  u32 hash = Hash(src, len);
  Val key = SymVal(hash);

  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (Eq(vm->symbols[i].key, key)) {
      return key;
    }
    if (IsNil(vm->symbols[i].key)) {
      vm->symbols[i].key = key;
      vm->symbols[i].name = StrToBinary(vm, src, len);
      return key;
    }
  }

  Error("Too many symbols");
}

Val SymbolName(VM *vm, Val sym)
{
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (Eq(vm->symbols[i].key, sym)) {
      return vm->symbols[i].name;
    }
  }

  Error("Symbol not found");
}

Val SymbolFor(VM *vm, char *src, u32 len)
{
  u32 hash = Hash(src, len);
  return SymVal(hash);
}

Val BinToSymbol(VM *vm, Val bin)
{
  byte *data = BinaryData(vm, bin);
  u32 length = BinaryLength(vm, bin);
  u32 hash = Hash(data, length);
  Val key = SymVal(hash);

  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (Eq(vm->symbols[i].key, key)) {
      return key;
    }
    if (IsNil(vm->symbols[i].key)) {
      vm->symbols[i].key = key;
      vm->symbols[i].name = bin;
      return key;
    }
  }

  Error("Too many symbols");
}

Val MakePair(VM *vm, Val head, Val tail)
{
  u32 index = Allocate(vm, 2);
  vm->mem[vm->base + index] = head;
  vm->mem[vm->base + index+1] = tail;

  return PairVal(index);
}

Val Head(VM *vm, Val pair)
{
  u32 index = RawVal(pair);
  return vm->mem[vm->base + index];
}

Val Tail(VM *vm, Val pair)
{
  u32 index = RawVal(pair);
  return vm->mem[vm->base + index+1];
}

void SetHead(VM *vm, Val pair, Val head)
{
  if (IsNil(pair)) {
    Error("Cannot change nil");
  }
  u32 index = RawVal(pair);
  vm->mem[vm->base + index] = head;
}

void SetTail(VM *vm, Val pair, Val tail)
{
  if (IsNil(pair)) {
    Error("Cannot change nil");
  }
  u32 index = RawVal(pair);
  vm->mem[vm->base + index+1] = tail;
}

Val ReverseOnto(VM *vm, Val list, Val tail)
{
  if (IsNil(list)) return tail;

  Val rest = Tail(vm, list);
  SetTail(vm, list, tail);
  return ReverseOnto(vm, rest, list);
}

Val Reverse(VM *vm, Val list)
{
  return ReverseOnto(vm, list, nil_val);
}

Val MakeTuple(VM *vm, u32 length, ...)
{
  va_list args;

  u32 index = Allocate(vm, length + 1);
  vm->mem[vm->base + index] = IntVal(length);

  va_start(args, length);
  for (u32 i = 0; i < length; i++) {
    Val arg = va_arg(args, Val);
    vm->mem[vm->base + index + 1 + i] = arg;
  }

  va_end(args);

  return TupleVal(index);
}

u32 TupleLength(VM *vm, Val tuple)
{
  u32 index = RawVal(tuple);
  return RawVal(vm->mem[vm->base + index]);
}

Val TupleAt(VM *vm, Val tuple, u32 i)
{
  u32 index = RawVal(tuple);
  return vm->mem[vm->base + index + 1 + i];
}

Val StrToBinary(VM *vm, char *src, u32 len)
{
  Val bin = MakeBinary(vm, len);
  u32 words = (len - 1) / sizeof(u32) + 1;
  u32 index = RawVal(bin);

  u32 *data = (u32*)src;
  u32 *word_base = (u32*)(vm->mem + vm->base + index + 1);
  for (u32 i = 0; i < words - 1; i++) {
    word_base[i] = data[i];
  }
  byte *base = (byte*)(vm->mem + vm->base + index + 1);
  for (u32 i = sizeof(u32)*(words - 1); i < len; i++) {
    base[i] = src[i];
  }

  return bin;
}

Val MakeBinary(VM *vm, u32 len)
{
  u32 words = (len - 1) / sizeof(u32) + 1;
  u32 index = Allocate(vm, words + 1);
  vm->mem[vm->base + index] = IntVal(len);

  return BinVal(index);
}

u32 BinaryLength(VM *vm, Val binary)
{
  u32 index = RawVal(binary);
  return RawVal(vm->mem[vm->base + index]);
}

byte *BinaryData(VM *vm, Val binary)
{
  u32 index = RawVal(binary);
  return (byte*)(vm->mem + vm->base + index + 1);
}

void DumpVM(VM *vm)
{
  printf("┌──────────────────────────────────────────────────────────────────────┐\n");
  printf("│ \x1b[4mRegisters\x1b[0m                                                            │\n");
  printf("│ exp:  0x%08X  env:  0x%08X                                   │\n", vm->exp, vm->env);
  printf("│ proc: 0x%08X  argl: 0x%08X                                   │\n", vm->proc, vm->argl);
  printf("│ val:  0x%08X  cont: 0x%08X                                   │\n", vm->val.as_v, vm->cont);
  printf("│ unev: 0x%08X  sp:   0x%08X                                   │\n", vm->unev, vm->sp);
  printf("│ base: 0x%08X  free: 0x%08X                                   │\n", vm->base, vm->free);
  printf("│                                                                      │\n");

  printf("│ \x1b[4mMemory\x1b[0m                                                               │\n");
  u32 ncols = 4;
  for (u32 i = vm->base; i < vm->free; i += ncols) {
    printf("│ 0x%08X  ", i);
    for (u32 j = 0; j < ncols && i + j < vm->free; j++) {
      printf("%08X ", vm->mem[i+j].as_v);
    }
    if (i + ncols > vm->free) {
      for (u32 j = vm->free; j % ncols != 0; j++) {
        printf("         ");
      }
    }
    printf(" ");
    for (u32 j = 0; j < ncols && i + j < vm->free; j++) {
      printf("%c%c%c%c ", Chr(A(vm->mem[i+j].as_v)), Chr(B(vm->mem[i+j].as_v)), Chr(C(vm->mem[i+j].as_v)), Chr(D(vm->mem[i+j].as_v)));
    }

    if (i + ncols > vm->free) {
      for (u32 j = vm->free; j % ncols != 0; j++) {
        printf("     ");
      }
    }
    printf("│\n");
  }
  printf("│                                                                      │\n");

  printf("│ \x1b[4mStack\x1b[0m                                                                │\n");
  for (u32 i = MEMORY_SIZE - 1; i > vm->sp; i--) {
    printf("│ 0x%08X  %08X %c%c%c%c                                            │\n", i, vm->mem[i].as_v, Chr(A(vm->mem[i].as_v)), Chr(B(vm->mem[i].as_v)), Chr(C(vm->mem[i].as_v)), Chr(D(vm->mem[i].as_v)));
  }
  printf("│                                                                      │\n");

  printf("│ \x1b[4mSymbols\x1b[0m                                                              │\n");
  for (u32 i = 0; i < NUM_SYMBOLS; i++) {
    if (IsNil(vm->symbols[i].key)) {
      break;
    }

    u32 len = BinaryLength(vm, vm->symbols[i].name);
    char *data = (char*)BinaryData(vm, vm->symbols[i].name);
    printf("│ 0x%08X    ", vm->symbols[i].key.as_v);
    for (u32 i = 0; i < len; i++) {
      printf("%c", data[i]);
    }
    u32 num_spaces = (len > 55) ? 0 : 55 - len;
    for (u32 i = 0; i < num_spaces; i++) printf(" ");
    printf("│\n");
  }
  printf("└──────────────────────────────────────────────────────────────────────┘\n");
}

void PrintTail(VM *vm, Val tail)
{
  if (IsNil(tail)) {
    printf(")");
    return;
  }

  printf(" ");
  PrintValue(vm, Head(vm, tail));
  PrintTail(vm, Tail(vm, tail));
}

void PrintValue(VM *vm, Val val)
{
  switch (TypeOf(val)) {
  case NUMBER:
    printf("%.1f", (float)RawVal(val));
    break;
  case INTEGER:
    printf("%d", (u32)RawVal(val));
    break;
  case PAIR:
    printf("(");
    if (IsNil(val)) {
      printf(")");
    } else {
      PrintValue(vm, Head(vm, val));
      if (IsPair(Tail(vm, val))) {
        PrintTail(vm, Tail(vm, val));
      } else {
        printf(" . ");
        PrintValue(vm, Tail(vm, val));
        printf(")");
      }
    }
    break;
  case SYMBOL:
    for (u32 i = 0; i < BinaryLength(vm, SymbolName(vm, val)); i++) {
      printf("%c", BinaryData(vm, SymbolName(vm, val))[i]);
    }
    break;
  case BINARY:
    printf("\"");
    for (u32 i = 0; i < BinaryLength(vm, val); i++) {
      printf("%c", ((char*)BinaryData(vm, val))[i]);
    }
    printf("\"");
    break;
  case TUPLE:
    printf("[");
    for (u32 i = 0; i < TupleLength(vm, val); i++) {
      PrintValue(vm, TupleAt(vm, val, i));
      if (i < TupleLength(vm, val) - 1) printf(" ");
    }
    printf("]");
    break;
  default:
    printf("0x%08x", val.as_v);
  }
}
