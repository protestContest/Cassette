#include "vec.h"
#include "math.h"
#include <stdlib.h>

static void ResizeVec(Vec *vec, u32 item_size, u32 capacity);
static void MaybeGrowVec(Vec *vec, u32 item_size);

void InitVec(Vec *vec, u32 item_size, u32 capacity)
{
  vec->capacity = 0;
  vec->count = 0;
  vec->items = 0;
  ResizeVec(vec, item_size, capacity);
}

void DestroyVec(Vec *vec)
{
  if (vec->items) free(vec->items);
  vec->capacity = 0;
  vec->count = 0;
}

u32 GrowVec(Vec *vec, u32 item_size, u32 amount)
{
  u32 old_count = vec->count;
  if (vec->count + amount < vec->capacity) {
    ResizeVec(vec, item_size, Max(2*vec->capacity, vec->count + amount));
  }
  vec->count += amount;
  return old_count;
}

void IntVecPush(IntVec *vec, u32 value)
{
  MaybeGrowVec((Vec*)vec, sizeof(value));
  vec->items[vec->count++] = value;
}

void ByteVecPush(ByteVec *vec, u8 value)
{
  MaybeGrowVec((Vec*)vec, sizeof(value));
  vec->items[vec->count++] = value;
}

void ObjVecPush(ObjVec *vec, void *value)
{
  MaybeGrowVec((Vec*)vec, sizeof(value));
  vec->items[vec->count++] = value;
}

static void ResizeVec(Vec *vec, u32 item_size, u32 capacity)
{
  vec->capacity = capacity;
  vec->items = realloc(vec->items, item_size*capacity);
}

static void MaybeGrowVec(Vec *vec, u32 item_size)
{
  if (vec->count >= vec->capacity) {
    ResizeVec((Vec*)vec, item_size, 2*vec->capacity);
  }
}
