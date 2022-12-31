#include "mem.h"
#include "printer.h"
#include "hash.h"
#include "env.h"

#define MEM_SIZE (4096*4096)
#define GC_THRESHHOLD 0.5
Val mem[MEM_SIZE];
u32 mem_next = 2;
#define mem_base      (u32)RawVal(nil)

typedef struct {
  Val key;
  char *name;
} Symbol;

#define NUM_SYMBOLS 256
Symbol symbols[NUM_SYMBOLS];
u32 sym_next = 0;

Val nil = PairVal(0);
Val root = PairVal(0);

u32 MemUsed(void)
{
  if (mem_base <= mem_next) {
    return mem_next - mem_base;
  } else {
    return MEM_SIZE - mem_base + mem_next;
  }
}

void InitMem(void)
{
  SetHead(nil, nil);
  SetTail(nil, nil);
  root = MakeTuple(3, nil, nil, InitialEnv());
}

Val MakePair(Val head, Val tail)
{
  if (mem_next+1 >= MEM_SIZE) mem_next = 0;
  if (mem_base >= mem_next && mem_base < mem_next + 2) {
    DumpMem();
    Error("Out of memory");
  }

  Val pair = PairVal(mem_next);

  mem[mem_next++] = head;
  mem[mem_next++] = tail;

  if (MemUsed() >= GC_THRESHHOLD*MEM_SIZE) {
    // GarbageCollect();
  }

  return pair;
}

Val Head(Val pair)
{
  u32 index = RawVal(pair);
  return mem[index];
}

Val Tail(Val pair)
{
  u32 index = RawVal(pair);
  return mem[index+1];
}

void SetHead(Val pair, Val val)
{
  u32 index = RawVal(pair);
  mem[index] = val;
}

void SetTail(Val pair, Val val)
{
  u32 index = RawVal(pair);
  mem[index+1] = val;
}

Val ReverseOnto(Val list, Val tail)
{
  if (IsNil(list)) return tail;

  Val rest = Tail(list);
  SetTail(list, tail);
  return ReverseOnto(rest, list);
}

Val Reverse(Val list)
{
  return ReverseOnto(list, nil);
}

Val MakeTuple(u32 count, ...)
{
  if (mem_next+1 >= MEM_SIZE) mem_next = 0;
  if (mem_base >= mem_next && mem_base < mem_next + count + 1) {
    DumpMem();
    Error("Out of memory");
  }

  Val tuple = TupleVal(mem_next);
  mem[mem_next++] = HdrVal(count);

  if (count == 0) {
    mem[mem_next++] = nil;
    return tuple;
  }

  va_list args;
  va_start(args, count);
  for (u32 i = 0; i < count; i++) {
    Val arg = va_arg(args, Val);
    mem[mem_next++] = arg;
    if (mem_next == MEM_SIZE) mem_next = 0;
  }
  va_end(args);

  return tuple;
}

Val TupleLength(Val tuple)
{
  u32 index = RawVal(tuple);
  return mem[index];
}

Val TupleAt(Val tuple, u32 i)
{
  u32 index = RawVal(tuple);
  return mem[(index + i + 1) % MEM_SIZE];
}

void TupleSet(Val tuple, u32 i, Val val)
{
  u32 index = RawVal(tuple);
  mem[(index + i + 1) % MEM_SIZE] = val;
}

bool IsTagged(Val exp, Val tag)
{
  if (IsPair(exp) && Eq(Head(exp), tag)) return true;
  if (IsTuple(exp) && Eq(TupleAt(exp, 0), tag)) return true;
  return false;
}

Val MakeSymbol(char *src, u32 len)
{
  Val key = SymVal(Hash(src, len));

  for (u32 i = 0; i < sym_next; i++) {
    if (Eq(symbols[i].key, key)) {
      return key;
    }
  }

  if (sym_next >= NUM_SYMBOLS) Error("Too many symbols");

  Symbol *sym = &symbols[sym_next++];
  sym->key = key;
  sym->name = malloc(len+1);
  memcpy(sym->name, src, len);
  sym->name[len] = '\0';
  return key;
}

char *SymbolName(Val sym)
{
  for (u32 i = 0; i < sym_next; i++) {
    if (Eq(symbols[i].key, sym)) {
      return symbols[i].name;
    }
  }
  return NULL;
}

Val Relocate(Val obj)
{
  if (IsPair(obj)) {
    if (mem_next+1 >= MEM_SIZE) mem_next = 0;
    Val pair = PairVal(mem_next);
    mem[mem_next++] = Head(obj);
    mem[mem_next++] = Tail(obj);
    SetHead(obj, MakeSymbol("moved", 5));
    SetTail(obj, pair);
    return pair;
  } else if (IsTuple(obj)) {
    u32 count = RawVal(TupleLength(obj));
    Val tuple = MakeTuple(count);
    if (count == 0) {
      TupleSet(tuple, 0, nil);
    } else {
      for (u32 i = 0; i < count; i++) {
        TupleSet(tuple, i, TupleAt(obj, i));
      }
    }

    SetHead(obj, MakeSymbol("moved", 5));
    SetTail(obj, tuple);
    return tuple;
  }


  return obj;
}

Val GarbageCollect(void)
{
  nil = Relocate(nil);
  root = Relocate(root);

  u32 scan = RawVal(nil);

  while (scan != mem_next) {
    if (IsPair(mem[scan]) || IsTuple(mem[scan])) {
      Val obj = mem[scan];
      if (!Eq(Head(obj), MakeSymbol("moved", 5))) {
        Relocate(obj);
      }
      mem[scan] = Tail(obj);
    }
    scan++;
    if (scan > MEM_SIZE) scan = 0;
  }

  return root;
}

void DumpMem(void)
{
  printf("Mem (%d-%d)\n", mem_base, mem_next);
  u32 i = 0;
  while (i < MEM_SIZE) {
    if (IsHdr(mem[i])) {
      printf("  %04d    0x%08X    ", i, mem[i].as_v);
      u32 size = RawVal(mem[i]);
      printf("┌ Obj [%d]\n", size);
      for (u32 j = 0; j < size; j++) {
        printf("  %04d    0x%08X    ", i+j, mem[i+1+j].as_v);
        if (j == size-1) printf("└ ");
        else printf("│ ");
        DebugVal(mem[i+j+1]);
        printf("\n");
      }
      i += size + 1;
    } else {
      printf("  %04d    0x%08X    ", i, mem[i].as_v);
      printf("┌ ");
      DebugVal(mem[i]);
      printf("\n");
      printf("  %04d    0x%08X    ", i+1, mem[i+1].as_v);
      printf("└ ");
      DebugVal(mem[i+1]);
      printf("\n");
      i += 2;
    }
  }
}

void DumpSymbols(void)
{
  printf("Symbols\n");
  for (u32 i = 0; i < sym_next; i++) {
    printf("  0x%0X %s\n", symbols[i].key.as_v, symbols[i].name);
  }
}
