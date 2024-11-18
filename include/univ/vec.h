#pragma once
#include "univ/handle.h"

typedef struct {
  u32 count;
  void *data[];
} Vec;

#define VecOf(type) \
struct { \
  u32 count; \
  type data[]; \
}

typedef VecOf(u8) **ByteVec;
typedef VecOf(u32) **WordVec;

#define NewVec(type, count)   (void*)MakeVec(count, sizeof(type))
#define FreeVec(v)            DisposeHandle((Handle)v)
#define InitVec(v)            (v) = (void*)MakeVec(0, 0)
#define VecData(v)            ((*(v))->data)
#define ItemSize(v)           sizeof(*VecData(v)) /* NOLINT */
#define VecCount(v)           (**((u32**)(v)))
#define VecCap(v)             ((GetHandleSize((Handle)v) - sizeof(Vec)) / ItemSize(v)) /* NOLINT */
#define VecAt(v,i)            (VecData(v)[i])
#define VecEnd(v)             (VecData(v) + VecCount(v))
#define VecDelete(v,i)        DoVecDelete(v, i, ItemSize(v))
#define GrowVec(v,c) do {\
  if (VecCount(v) + (c) > VecCap(v)) {\
    SetHandleSize((Handle)v, sizeof(Vec) + Max(VecCount(v)+(c), 2*VecCap(v))*ItemSize(v)); /* NOLINT */\
  }\
  VecCount(v) += (c);\
} while (0)

#define VecPush(v,x) do {\
  GrowVec(v,1);\
  VecAt(v, VecCount(v)-1) = (x);\
} while (0)

#define VecPop(v)             (VecData(v)[--VecCount(v)])

Handle MakeVec(u32 count, u32 itemSize);
void DoVecDelete(Handle vec, u32 index, u32 itemSize);
