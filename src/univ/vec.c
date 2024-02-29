#include "vec.h"
#include "math.h"
#include "univ/system.h"

static void MaybeGrowVec(Vec *vec, u32 item_size);

void InitVec(void *vec, u32 item_size, u32 capacity)
{
  ((Vec*)vec)->capacity = 0;
  ((Vec*)vec)->count = 0;
  ((Vec*)vec)->items = 0;
  ResizeVec(vec, item_size, capacity);
}

void DestroyVec(void *vec)
{
  if (((Vec*)vec)->items) Free(((Vec*)vec)->items);
  ((Vec*)vec)->items = 0;
  ((Vec*)vec)->capacity = 0;
  ((Vec*)vec)->count = 0;
}

u32 GrowVec(void *vec, u32 item_size, u32 amount)
{
  u32 old_count = ((Vec*)vec)->count;
  if (((Vec*)vec)->count + amount >= ((Vec*)vec)->capacity) {
    u32 capacity = Max(2*((Vec*)vec)->capacity, ((Vec*)vec)->count + amount);
    ResizeVec(vec, item_size, capacity);
  }
  ((Vec*)vec)->count += amount;
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

void ResizeVec(void *vec, u32 item_size, u32 capacity)
{
  ((Vec*)vec)->capacity = capacity;
  ((Vec*)vec)->items = Realloc(((Vec*)vec)->items, item_size*capacity);
}

static void MaybeGrowVec(Vec *vec, u32 item_size)
{
  if (vec->count >= vec->capacity) {
    ResizeVec((Vec*)vec, item_size, Max(4, 2*vec->capacity));
  }
}
