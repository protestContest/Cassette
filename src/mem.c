#include "mem.h"
#include "symbol.h"

typedef struct {
  u32 item_size;
  u32 capacity;
  u32 count;
  void *data;
} Vector;

typedef struct {
  Vector pairs;
  Vector vecs;
} MemState;

static MemState mem = {
  {sizeof(Pair), 0, 0, NULL},
  {sizeof(Vector), 0, 0, NULL}
};

#define INITIAL_NUM_PAIRS 32
#define VEC_AS(vec, type)   ((type*)((vec).data))
#define PAIRS               VEC_AS(mem.pairs, Pair)
#define VECS                VEC_AS(mem.vecs, Vector)

void ResizeVector(Vector *vec, u32 count);

void InitMem(void)
{
  ResizeVector(&mem.pairs, INITIAL_NUM_PAIRS);
}

Value MakePair(Value head, Value tail)
{
  if (mem.pairs.count >= mem.pairs.capacity) {
    ResizeVector(&mem.pairs, 2*mem.pairs.capacity);
  }

  u32 pos = mem.pairs.count++;
  PAIRS[pos].head = head;
  PAIRS[pos].tail = tail;

  Value val = PairVal(pos);
  return val;
}

Value Head(Value obj)
{
  return PAIRS[AsVec(obj)].head;
}

Value Tail(Value obj)
{
  return PAIRS[AsVec(obj)].tail;
}

void SetHead(Value obj, Value head)
{
  PAIRS[AsVec(obj)].head = head;
}

void SetTail(Value obj, Value tail)
{
  PAIRS[AsVec(obj)].tail = tail;
}

Value MakeVec(u32 count)
{
  if (mem.vecs.count >= mem.vecs.capacity) {
    ResizeVector(&mem.vecs, 2*mem.vecs.capacity);
  }

  u32 pos = mem.vecs.count++;
  VECS[pos].item_size = sizeof(Value);
  VECS[pos].count = 0;
  VECS[pos].capacity = 0;
  VECS[pos].data = NULL;
  ResizeVector(&VECS[pos], count);

  Value val = VecVal(pos);
  return val;
}

Value VecGet(Value vec, u32 n)
{
  if (AsVec(vec) >= mem.vecs.count) {
    fprintf(stderr, "Invalid vector access\n");
    exit(1);
  }

  Vector v = VECS[AsVec(vec)];
  if (n >= v.count) {
    fprintf(stderr, "Invalid vector element access\n");
    exit(1);
  }

  return VEC_AS(v, Value)[n];
}

void VecSet(Value vec, u32 n, Value val)
{
  if (AsVec(vec) >= mem.vecs.count) {
    fprintf(stderr, "Invalid vector access\n");
    exit(1);
  }

  Vector v = VECS[AsVec(vec)];
  if (n >= v.count) {
    fprintf(stderr, "Invalid vector element access\n");
    exit(1);
  }

  VEC_AS(v, Value)[n] = val;
}

void ResizeVector(Vector *vec, u32 count)
{
  vec->data = realloc(vec->data, count*vec->item_size);
  vec->capacity = count;
}

void DumpPairs(void)
{
  if (mem.pairs.count == 0) {
    printf("  \x1B[4mMemory empty\x1B[0m\n");
    return;
  }

  const u32 min_len = 4;

  u32 len = LongestSymLength() > min_len ? LongestSymLength() : min_len;
  u32 table_width = 2*len + 14;

  printf("  \x1B[4m⚭ Pairs");
  for (u32 i = 0; i < table_width - 6; i++) printf(" ");
  printf("\x1B[0m\n");
  for (u32 i = 0; i < mem.pairs.count; i++) {
    printf("  ");
    Value head = PAIRS[i].head;
    Value tail = PAIRS[i].tail;
    printf("% 4d │ ", i);
    PrintValue(head, len);
    printf(" │ ");
    PrintValue(tail, len);
    printf("\n");
  }
  printf("\n");
}
