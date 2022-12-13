#include "mem.h"
#include "symbol.h"

typedef struct {
  Pair *pairs;
  u32 next;
  u32 total;
} MemState;

static MemState mem = {NULL, 0, 0};

void InitMem(u32 num_pairs)
{
  if (mem.pairs != NULL) free(mem.pairs);
  mem.pairs = calloc(num_pairs, sizeof(Pair));
  mem.next = 0;
  mem.total = num_pairs;

  // nil
  MakePair(Index(0), Index(0));
}

void *GetMemState(void)
{
  return &mem;
}

Value MakePair(Value head, Value tail)
{
  if (mem.next >= mem.total) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }

  u32 pos = mem.next++;
  mem.pairs[pos].head = head;
  mem.pairs[pos].tail = tail;
  Value v = Index(pos);
  return v;
}

Value Head(Value obj)
{
  return mem.pairs[AsIndex(obj)].head;
}

Value Tail(Value obj)
{
  return mem.pairs[AsIndex(obj)].tail;
}

void SetHead(Value obj, Value head)
{
  mem.pairs[AsIndex(obj)].head = head;
}

void SetTail(Value obj, Value tail)
{
  mem.pairs[AsIndex(obj)].tail = tail;
}

void DumpMem(void)
{
  if (mem.next == 0) {
    printf("Memory empty\n");
    return;
  }

  u32 len = LongestSymLength() > 8 ? LongestSymLength() : 8;
  u32 table_width = 2*len + 14;

  printf("\n");
  printf("  \x1B[4mMemory");
  for (u32 i = 0; i < table_width - 6; i++) printf(" ");
  printf("\x1B[0m\n");
  for (u32 i = 0; i < mem.next; i++) {
    printf("  ");
    Value head = mem.pairs[i].head;
    Value tail = mem.pairs[i].tail;
    printf("% 3d │ %s ", i, TypeAbbr(head));
    PrintValue(head, len);
    printf(" │ %s ", TypeAbbr(tail));
    PrintValue(tail, len);
    printf("\n");
  }
}
