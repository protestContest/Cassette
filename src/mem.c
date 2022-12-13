#include "mem.h"
#include "symbol.h"

static Pair *mem = NULL;
static u32 next = 0;
static u32 total = 0;

void InitMem(u32 num_pairs)
{
  if (mem != NULL) free(mem);
  next = 0;
  total = num_pairs;
  mem = calloc(total, sizeof(Pair));

  // nil
  MakePair(Index(0), Index(0));
}

Value MakePair(Value head, Value tail)
{
  if (next >= total) {
    fprintf(stderr, "Out of memory\n");
    exit(1);
  }

  u32 pos = next++;
  mem[pos].head = head;
  mem[pos].tail = tail;
  Value v = Index(pos);
  return v;
}

Value Head(Value obj)
{
  return mem[AsIndex(obj)].head;
}

Value Tail(Value obj)
{
  return mem[AsIndex(obj)].tail;
}

void SetHead(Value obj, Value head)
{
  mem[AsIndex(obj)].head = head;
}

void SetTail(Value obj, Value tail)
{
  mem[AsIndex(obj)].tail = tail;
}

char TypeAbbr(Value value)
{
  switch (TypeOf(value)) {
  case NUMBER:  return 'N';
  case SYMBOL:  return 'S';
  case OBJ:     return 'O';
  default:      return '?';
  }
}

void PrintMem(void)
{
  if (next == 0) {
    printf("Memory empty\n");
    return;
  }

  printf("\nMemory:\n");
  for (u32 i = 0; i < next; i++) {
    Value head = mem[i].head;
    Value tail = mem[i].tail;
    printf("% 3d | %c ", i, TypeAbbr(head));
    PrintValue(head);
    printf(" | %c ", TypeAbbr(tail));
    PrintValue(tail);
    printf("\n");
  }
}
