#include "mem.h"
#include "symbol.h"
#include "debug.h"

typedef struct {
  Value head;
  Value tail;
} Pair;

typedef struct {
  u32 item_size;
  u32 capacity;
  u32 count;
  void *data;
} Vector;

typedef struct {
  Vector pairs;
  Vector heap;
} MemState;

static MemState mem = {
  {sizeof(Pair), 0, 0, NULL},
  {sizeof(ObjHeader), 0, 0, NULL}
};

#define INITIAL_NUM_PAIRS 32
#define INITIAL_HEAP_SIZE 1024

#define VEC_AS(vec, type)   ((type*)((vec).data))
#define PAIRS               VEC_AS(mem.pairs, Pair)
#define HEAP                VEC_AS(mem.heap, ObjHeader)

void ResizeVector(Vector *vec, u32 count);
void FreeVector(Vector *vec);

void InitMem(void)
{
  ResizeVector(&mem.pairs, INITIAL_NUM_PAIRS);
  ResizeVector(&mem.heap, INITIAL_HEAP_SIZE);
  InitSymbols();
}

void ResetMem(void)
{
  FreeVector(&mem.pairs);
  FreeVector(&mem.heap);
  InitMem();
  ResetSymbols();
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

Value Head(Value pair)
{
  return PAIRS[RawVal(pair)].head;
}

Value Tail(Value pair)
{
  return PAIRS[RawVal(pair)].tail;
}

void SetHead(Value pair, Value head)
{
  PAIRS[RawVal(pair)].head = head;
}

void SetTail(Value pair, Value tail)
{
  PAIRS[RawVal(pair)].tail = tail;
}

void ResizeVector(Vector *vec, u32 count)
{
  vec->data = realloc(vec->data, count*vec->item_size);
  vec->capacity = count;
}

void FreeVector(Vector *vec)
{
  if (vec->data != NULL) free(vec->data);
  vec->capacity = 0;
  vec->count = 0;
}

void CheckVectorSize(Vector *vec, u32 count)
{
  u32 need = vec->count + count;
  if (need >= vec->capacity) {
    u32 new_count = (need > 2*vec->capacity) ? need : 2*vec->capacity;
    ResizeVector(vec, new_count);
  }
}

u32 Allocate(u32 size)
{
  CheckVectorSize(&mem.heap, size);
  u32 ptr = mem.heap.count;
  mem.heap.count += size;
  return ptr;
}

u32 AllocateObject(ObjHeader header, u32 size)
{
  u32 ptr = Allocate(size + sizeof(ObjType));
  ((ObjHeader*)mem.heap.data)[ptr] = header;
  return ptr;
}

ObjHeader *ObjectRef(Value value)
{
  return &HEAP[RawVal(value)];
}

void DumpPairs(void)
{
  const u32 min_len = 8;

  u32 len = LongestSymLength() + 2 > min_len ? LongestSymLength() + 2 : min_len;
  u32 table_width = 2*len + 4;

  DebugTable("⚭ Pairs", table_width, 3);

  if (mem.pairs.count == 0) {
    DebugEmpty();
    return;
  }

  for (u32 i = 0; i < mem.pairs.count; i++) {
    Value head = PAIRS[i].head;
    Value tail = PAIRS[i].tail;

    DebugRow();
    printf("% 4d", i);
    DebugCol();
    DebugValue(head, len);
    DebugCol();
    DebugValue(tail, len);
  }
  EndDebugTable();
}
