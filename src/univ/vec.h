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

void InitVec(Vec *vec, u32 item_size, u32 capacity);
void DestroyVec(Vec *vec);
u32 GrowVec(Vec *vec, u32 item_size, u32 amount);
void VecPop(Vec *vec);

void IntVecPush(IntVec *vec, u32 value);
void ByteVecPush(ByteVec *vec, u8 value);
void ObjVecPush(ObjVec *vec, void *value);
