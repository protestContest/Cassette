#pragma once

/* A simple dynamic array implementation. Each type has the same structure, but
   is more convenient to access */

typedef struct {
  u32 capacity;
  u32 count;
  void *items;
} Vec;

typedef struct {
  u32 capacity;
  u32 count;
  u32 *items;
} IntVec;

typedef struct {
  u32 capacity;
  u32 count;
  u8 *items;
} ByteVec;

typedef struct {
  u32 capacity;
  u32 count;
  void **items;
} ObjVec;

#define VecRef(vec, i)      ((vec)->items[i])
#define CapacityLeft(vec)   ((vec)->capacity - (vec)->count)
#define VecPop(vec)         ((vec)->items[--(vec)->count])
#define VecPeek(vec, i)     ((vec)->items[(vec)->count - (i) - 1])

void InitVec(void *vec, u32 item_size, u32 capacity);
void DestroyVec(void *vec);
void ResizeVec(void *vec, u32 item_size, u32 capacity);
u32 GrowVec(void *vec, u32 item_size, u32 amount);

void IntVecPush(IntVec *vec, u32 value);
void ByteVecPush(ByteVec *vec, u8 value);
void ObjVecPush(ObjVec *vec, void *value);
