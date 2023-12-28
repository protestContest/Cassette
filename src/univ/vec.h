#pragma once

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

void InitVec(Vec *vec, u32 item_size, u32 capacity);
void DestroyVec(Vec *vec);
void ResizeVec(Vec *vec, u32 item_size, u32 capacity);
u32 GrowVec(Vec *vec, u32 item_size, u32 amount);


void IntVecPush(IntVec *vec, u32 value);
void ByteVecPush(ByteVec *vec, u8 value);
void ObjVecPush(ObjVec *vec, void *value);

